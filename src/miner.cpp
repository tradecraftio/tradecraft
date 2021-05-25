// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <miner.h>

#include <amount.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <hash.h>
#include <key_io.h>
#include <net.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <pow.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <timedata.h>
#include <txmempool.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <validationinterface.h>
#ifdef ENABLE_WALLET
#include <base58.h>
#include <script/sign.h>
#include <wallet/wallet.h>
#endif

#include <algorithm>
#include <queue>
#include <utility>

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);

    return nNewTime - nOldTime;
}

BlockAssembler::Options::Options() {
    blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
}

BlockAssembler::BlockAssembler(const CChainParams& params, const Options& options) : chainparams(params)
{
    blockMinFeeRate = options.blockMinFeeRate;
    // Limit weight to between 4K and MAX_BLOCK_WEIGHT-4K for sanity:
    nBlockMaxWeight = std::max<size_t>(4000, std::min<size_t>(MAX_BLOCK_WEIGHT - 4000, options.nBlockMaxWeight));

    nMedianTimePast = 0;
    block_final_state = NO_BLOCK_FINAL_TX;
}

static BlockAssembler::Options DefaultOptions()
{
    // Block resource limits
    // If -blockmaxweight is not given, limit to DEFAULT_BLOCK_MAX_WEIGHT
    BlockAssembler::Options options;
    options.nBlockMaxWeight = gArgs.GetArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    CAmount n = 0;
    if (gArgs.IsArgSet("-blockmintxfee") && ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n)) {
        options.blockMinFeeRate = CFeeRate(n);
    } else {
        options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    }
    return options;
}

BlockAssembler::BlockAssembler(const CChainParams& params) : BlockAssembler(params, DefaultOptions()) {}

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockWeight = 4000;
    nBlockSigOpsCost = 400;
    fIncludeWitness = false;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;

    nMedianTimePast = 0;
    block_final_state = NO_BLOCK_FINAL_TX;
}

Optional<int64_t> BlockAssembler::m_last_block_num_txs{nullopt};
Optional<int64_t> BlockAssembler::m_last_block_weight{nullopt};

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn)
{
    int64_t nTimeStart = GetTimeMicros();

    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex* pindexPrev = chainActive.Tip();
    assert(pindexPrev != nullptr);
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = gArgs.GetArg("-blockversion", pblock->nVersion);

    pblock->nTime = GetAdjustedTime();
    nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                       ? nMedianTimePast
                       : pblock->GetBlockTime();


    // Check if block-final tx rules are enforced. For the moment this
    // tracks just whether the soft-fork is active, but by the time we get
    // to transaction selection it will only be true if there is a
    // block-final transaction in this block template.
    if (VersionBitsState(pindexPrev, chainparams.GetConsensus(), Consensus::DEPLOYMENT_FINALTX, versionbitscache) == ThresholdState::ACTIVE) {
        block_final_state = HAS_BLOCK_FINAL_TX;
    }

    // Check if this is the first block for which the block-final rules are
    // enforced, in which case all we need to do is add the initial
    // anyone-can-spend output.
    if ((block_final_state & HAS_BLOCK_FINAL_TX) && (!pindexPrev->pprev || (VersionBitsState(pindexPrev->pprev, chainparams.GetConsensus(), Consensus::DEPLOYMENT_FINALTX, versionbitscache) != ThresholdState::ACTIVE))) {
        block_final_state = INITIAL_BLOCK_FINAL_TXOUT;
    }

    // Otherwise we will need to check if the prior block-final transaction
    // was a coinbase and if insufficient blocks have occured for it to mature.
    BlockFinalTxEntry final_tx;
    if (block_final_state & HAS_BLOCK_FINAL_TX) {
        final_tx = pcoinsTip->GetFinalTx();
        if (final_tx.IsNull()) {
            // Should never happen
            return nullptr;
        }
        // Fetch the unspent outputs of the last block-final tx.  This call
        // should always return results because the prior block-final
        // transaction was the last processed transaction (so none of the
        // outputs could have been spent) or a previously immature coinbase.
        for (uint32_t n = 0; n < final_tx.size; ++n) {
            COutPoint prevout(final_tx.hash, n);
            const auto& coin = pcoinsTip->AccessCoin(prevout);
            if (coin.IsSpent()) {
                // Should never happen
                return nullptr;
            }
            // If it was a coinbase, meaning we're in the first 100 blocks after
            // activation, then we need to make sure it has matured, otherwise
            // we do nothing at all.
            if (coin.IsCoinBase() && (nHeight - coin.nHeight < COINBASE_MATURITY)) {
                // Still maturing. Nothing to do.
                block_final_state = NO_BLOCK_FINAL_TX;
                break;
            }
        }
    }

    // Decide whether to include witness transactions
    // This is only needed in case the witness softfork activation is reverted
    // (which would require a very deep reorganization).
    // Note that the mempool would accept transactions with witness data before
    // IsWitnessEnabled, but we would only ever mine blocks after IsWitnessEnabled
    // unless there is a massive block reorganization with the witness softfork
    // not activated.
    // TODO: replace this with a call to main to assess validity of a mempool
    // transaction (which in most cases can be a no-op).
    fIncludeWitness = IsWitnessEnabled(pindexPrev, chainparams.GetConsensus());

    initFinalTx(final_tx);

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
    addPackageTxs(nPackagesSelected, nDescendantsUpdated);

    int64_t nTime1 = GetTimeMicros();

    m_last_block_num_txs = nBlockTx;
    m_last_block_weight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
    coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    if (block_final_state & INITIAL_BLOCK_FINAL_TXOUT) {
        CTxOut txout(0, CScript() << OP_TRUE);
        coinbaseTx.vout.insert(coinbaseTx.vout.begin(), txout);
    }
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
    pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus());
    pblocktemplate->vTxFees[0] = -nFees;

    // The miner needs to know whether the last transaction is a special
    // transaction, or not.
    pblocktemplate->has_block_final_tx = (block_final_state & HAS_BLOCK_FINAL_TX);

    LogPrintf("CreateNewBlock(): block weight: %u txs: %u fees: %ld sigops %d\n", GetBlockWeight(*pblock), nBlockTx, (block_final_state & HAS_BLOCK_FINAL_TX) ? nFees - pblocktemplate->vTxFees.back() : nFees, nBlockSigOpsCost);

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus());
    pblock->nNonce         = 0;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);

    CValidationState state;
    if (!TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
        throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
    }
    int64_t nTime2 = GetTimeMicros();

    LogPrint(BCLog::BENCH, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n", 0.001 * (nTime1 - nTimeStart), nPackagesSelected, nDescendantsUpdated, 0.001 * (nTime2 - nTime1), 0.001 * (nTime2 - nTimeStart));

    return std::move(pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        }
        else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost) const
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight)
        return false;
    if (nBlockSigOpsCost + packageSigOpsCost >= MAX_BLOCK_SIGOPS_COST)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - premature witness (in case segwit transactions are added to mempool before
//   segwit activation)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package)
{
    for (CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
        if (!fIncludeWitness && it->GetTx().HasWitness())
            return false;
    }
    return true;
}

void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    // If we have a block-final transaction, insert just
    // before the end, so the block-final tx remains last.
    pblock->vtx.insert(pblock->vtx.end() - !!(block_final_state & HAS_BLOCK_FINAL_TX), iter->GetSharedTx());
    pblocktemplate->vTxFees.insert(pblocktemplate->vTxFees.end() - !!(block_final_state & HAS_BLOCK_FINAL_TX), iter->GetFee());
    pblocktemplate->vTxSigOpsCost.insert(pblocktemplate->vTxSigOpsCost.end() - !!(block_final_state & HAS_BLOCK_FINAL_TX), iter->GetSigOpCost());

    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee %s txid %s\n",
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}

int BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
        indexed_modified_transaction_set &mapModifiedTx)
{
    int nDescendantsUpdated = 0;
    for (CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc))
                continue;
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}

// Skip entries in mapTx that are already in a block or are present
// in mapModifiedTx (which implies that the mapTx ancestor state is
// stale due to ancestor inclusion in the block)
// Also skip transactions that we've already failed to add. This can happen if
// we consider a transaction in mapModifiedTx and it fails: we can then
// potentially consider it again while walking mapTx.  It's currently
// guaranteed to fail again, but as a belt-and-suspenders check we put it in
// failedTx and avoid re-evaluation, since the re-evaluation would be using
// cached size/sigops/fee values that are not actually correct.
bool BlockAssembler::SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx)
{
    assert (it != mempool.mapTx.end());
    return mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it);
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

static std::shared_ptr<CWallet> GetWalletForBlockFinalTx()
{
    if (!HasWallets()) {
        return nullptr;
    }
    // The user can configure which wallet to use for the block-final tx inputs
    // with the '-walletblockfinaltx' option.  By default, the default wallet is
    // used.
    const std::string requestedwallet = gArgs.GetArg("-walletblockfinaltx", "");
    std::shared_ptr<CWallet> ret = GetWallet(requestedwallet);
    if (ret) {
        return ret;
    }
    // If a wallet name was specified but not found, let's log that so the user
    // knows they need to fix their configuration.
    if (requestedwallet != "" && requestedwallet != "0") {
        LogPrintf("Requested wallet \"%s\" be used to source block-final transaction inputs, but no such wallet found.\n", requestedwallet);
    }
    // The user can disable use of wallet inputs for the block-final tx by
    // setting '-walletblockfinaltx' to 0 or false or '-nowalletblcokfinaltx=1'.
    // Note that there must not be a wallet named "0" (or "false", etc.), or
    // else that wallet will be selected above.
    if (!gArgs.GetBoolArg("-walletblockfinaltx", true)) {
        return nullptr;
    }
    // If we get this far, it is because the default wallet is requested.
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    return !wallets.empty() ? wallets[0] : nullptr;
}

void BlockAssembler::initFinalTx(const BlockFinalTxEntry& final_tx)
{
    // Create block-final tx
    CMutableTransaction txFinal;
    txFinal.nVersion = 2;
    txFinal.nLockTime = static_cast<uint32_t>(nMedianTimePast);

    // Block-final transactions are only created from prior block-final outputs
    // after we have reached the final state of activation.
    if (block_final_state & HAS_BLOCK_FINAL_TX) {
        // Add all outputs from the prior block-final transaction.  We do
        // nothing here to prevent selected transactions from spending these
        // same outputs out from underneath us; we depend insted on mempool
        // protections that prevent such transactions from being considered in
        // the first place.
        for (uint32_t n = 0; n < final_tx.size; ++n) {
            COutPoint prevout(final_tx.hash, n);
            const Coin& coin = pcoinsTip->AccessCoin(prevout);
            if (IsTriviallySpendable(coin, prevout, MANDATORY_SCRIPT_VERIFY_FLAGS|SCRIPT_VERIFY_WITNESS|SCRIPT_VERIFY_CLEANSTACK)) {
                txFinal.vin.push_back(CTxIn(prevout, CScript(), CTxIn::SEQUENCE_FINAL));
            } else {
                LogPrintf("WARNING: non-trivial output in block-final transaction record; this should never happen (%s:%n)\n", prevout.hash.ToString(), prevout.n);
            }
        }
    }

#ifdef ENABLE_WALLET
    else {
        if (!gArgs.GetBoolArg("-walletblockfinaltx", true)) {
            // User has requested that block-final transactions only be present
            // if the block-final rule change has activated.  The wallet will
            // not be used to generate a block-final transaction prior to
            // activation.
            return;
        }

        std::shared_ptr<CWallet> pwallet = GetWalletForBlockFinalTx();
        if (!pwallet) {
            LogPrintf("No wallet; unable to fetch outputs for block-final transaction.\n");
            return;
        }

        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);

        CTxDestination minesweep = DecodeDestination(gArgs.GetArg("-minesweepto", ""));
        CTxDestination carryforward = DecodeDestination(gArgs.GetArg("-carryforward", ""));

        // Get a vector of available outputs from the wallet.  This should not
        // include any outputs spent in this block, because outputs in the
        // mempool are excluded (and the transactions of the block were pulled
        // from the mempool).
        std::vector<COutput> outputs;
        pwallet->AvailableCoins(*locked_chain, outputs, false, nullptr, 0);
        if (outputs.empty()) {
            LogPrintf("No available wallet outputs for block-final transaction.\n");
            return;
        }

        // Gather inputs
        CAmount totalin = 0;
        for (const COutput& out : outputs) {
            if (!out.tx || out.nDepth<=0) {
                // Do not use unconfirmed outputs because we don't know if they
                // will be included in the block or not.
                continue;
            }
            txFinal.vin.emplace_back(COutPoint(out.tx->GetHash(), out.i));
            totalin += out.tx->tx->vout[out.i].nValue;
            if (!IsValidDestination(minesweep)) {
                // If we're not sweeping the wallet, then we only need one.
                break;
            }
        }
        // Optional: sweep outputs
        if (IsValidDestination(minesweep)) {
            // The block-final transaction already includes all
            // confirmed wallet outputs.  So we just have to add an
            // output to the minesweep address claiming the funds.
            txFinal.vout.emplace_back(totalin, GetScriptForDestination(minesweep));
            totalin = 0;
        }
        // Optional: fixed carry-forward addr
        if (IsValidDestination(carryforward)) {
            // If a carry-forward address is specified, we make sure
            // the tx includes an output to that address, to enable
            // future blocks to be mined.
            txFinal.vout.emplace_back(totalin, GetScriptForDestination(carryforward));
            totalin = 0;
        }
        // Default: send funds to reserve address
        if (totalin || !IsValidDestination(carryforward)) {
            std::shared_ptr<CReserveScript> reserve;
            pwallet->GetScriptForMining(reserve);
            if (!reserve) {
                LogPrintf("Keypool ran out while reserving script for block-final transaction, please call keypoolrefill\n");
                return;
            }
            if (reserve->reserveScript.empty()) {
                LogPrintf("No coinbase script available for block-final transaction (merge mining requires a wallet!)\n");
                return;
            }
            txFinal.vout.emplace_back(totalin, reserve->reserveScript);
            totalin = 0;
        }
    }
#endif // ENABLE_WALLET

    // We should have input(s) for the block-final transaction either from the
    // prior block-final transaction, or from the wallet.  If not, we cannot
    // create a block-final transaction because any non-coinbase transaction
    // must have valid inputs.
    if (txFinal.vin.empty()) {
        LogPrintf("Unable to create block-final transaction due to lack of inputs.\n");
        return;
    }

    // Add commitment and sign block-final transaction.
    if (!UpdateBlockFinalTransaction(txFinal, uint256())) {
        LogPrintf("Error signing block-final transaction; cannot use invalid transaction.\n");
        return;
    }

#ifdef ENABLE_WALLET
    // If the block-final transaction was created via the wallet, then we need
    // to update the state to indicate that the block-final transaction is
    // present.
    block_final_state |= HAS_BLOCK_FINAL_TX;
#endif // ENABLE_WALLET

    // Add block-final transaction to block template.
    pblock->vtx.emplace_back(MakeTransactionRef(std::move(txFinal)));

    // Record the fees forwarded by the block-final transaction to the coinbase.
    CAmount nTxFees = pcoinsTip->GetValueIn(*pblock->vtx.back())
                    - pblock->vtx.back()->GetValueOut();
    pblocktemplate->vTxFees.push_back(nTxFees);
    nFees += nTxFees;

    // The block-final transaction contributes to aggregate limits:
    // the number of sigops is tracked...
    int64_t nTxSigOpsCost = GetTransactionSigOpCost(*pblock->vtx.back(), *pcoinsTip, STANDARD_SCRIPT_VERIFY_FLAGS);
    pblocktemplate->vTxSigOpsCost.push_back(nTxSigOpsCost);
    nBlockSigOpsCost += nTxSigOpsCost;

    // ...the size is not:
    nBlockWeight += GetTransactionWeight(*pblock->vtx.back());
}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
void BlockAssembler::addPackageTxs(int &nPackagesSelected, int &nDescendantsUpdated)
{
    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;
    // Keep track of entries that failed inclusion, to avoid duplicate work
    CTxMemPool::setEntries failedTx;

    // Start by adding all descendants of previously added txs to mapModifiedTx
    // and modifying them for their already included ancestors
    UpdatePackagesForAdded(inBlock, mapModifiedTx);

    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;

    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;

    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty())
    {
        // First try to find a new transaction in mapTx to evaluate.
        if (mi != mempool.mapTx.get<ancestor_score>().end() &&
                SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {
            ++mi;
            continue;
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                    CompareTxMemPoolEntryByAncestorFee()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter));

        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmount packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->nModFeesWithAncestors;
            packageSigOpsCost = modit->nSigOpCostWithAncestors;
        }

        if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
            // Everything else we might consider has a lower fee rate
            return;
        }

        if (!TestPackage(packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }

            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
                    nBlockMaxWeight - 4000) {
                // Give up if we're close to full and haven't succeeded in a while
                break;
            }
            continue;
        }

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        // This transaction will make it in; reset the failed counter.
        nConsecutiveFailed = 0;

        // Package can be added. Sort the entries in a valid order.
        std::vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, sortedEntries);

        for (size_t i=0; i<sortedEntries.size(); ++i) {
            AddToBlock(sortedEntries[i]);
            // Erase from the modified set, if present
            mapModifiedTx.erase(sortedEntries[i]);
        }

        ++nPackagesSelected;

        // Update transactions that depend on each of these
        nDescendantsUpdated += UpdatePackagesForAdded(ancestors, mapModifiedTx);
    }
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

bool UpdateBlockFinalTransaction(CMutableTransaction &ret, const uint256& hash)
{
    CMutableTransaction mtx(ret);

    // Generate new commitment
    std::vector<unsigned char> new_commitment(36, 0x00);
    std::copy(hash.begin(), hash.end(),
              new_commitment.begin());
    new_commitment[32] = 0x4b;
    new_commitment[33] = 0x4a;
    new_commitment[34] = 0x49;
    new_commitment[35] = 0x48;

    // Find & update commitment
    if (!mtx.vout.empty() && mtx.vout.back().scriptPubKey.size() == 37 && mtx.vout.back().scriptPubKey[0] == 36 && mtx.vout.back().scriptPubKey[33] == 0x4b && mtx.vout.back().scriptPubKey[34] == 0x4a && mtx.vout.back().scriptPubKey[35] == 0x49 && mtx.vout.back().scriptPubKey[36] == 0x48) {
        mtx.vout.back().scriptPubKey = CScript() << new_commitment;
    } else {
        mtx.vout.emplace_back(0, CScript() << new_commitment);
    }

#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> pwallet = GetWalletForBlockFinalTx();
    if (pwallet) {
        LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);

        // Sign transaction
        CTransaction tx(mtx);
        ScriptError serror = SCRIPT_ERR_OK;
        for (size_t i = 0; i < tx.vin.size(); ++i) {
            CTxIn& txin = mtx.vin[i];
            const Coin& coin = pcoinsTip->AccessCoin(txin.prevout);
            if (coin.IsSpent()) {
                LogPrintf("Unable to find UTXO for block-final transaction input hash %s; unable to sign block-final transaction.\n", txin.prevout.hash.ToString());
                return false;
            }
            SignatureData sigdata;
            ProduceSignature(*pwallet, MutableTransactionSignatureCreator(&mtx, i, coin.out.nValue, SIGHASH_NONE), coin.out.scriptPubKey, sigdata);
            UpdateInput(mtx.vin.at(i), sigdata);
            ScriptError serror = SCRIPT_ERR_OK;
            if (!VerifyScript(txin.scriptSig, coin.out.scriptPubKey, &mtx.vin[i].scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&tx, i, coin.out.nValue), &serror)) {
                LogPrintf("error creating signature for wallet input to block-final transaction: %s", ScriptErrorString(serror));
                return false;
            }
        }
    }
#endif // ENABLE_WALLET

    ret = mtx;
    return true;
}
