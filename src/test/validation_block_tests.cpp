// Copyright (c) 2018-2019 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <boost/test/unit_test.hpp>

#include <test/setup_common.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <miner.h>
#include <pow.h>
#include <random.h>
#include <script/standard.h>
#include <test/setup_common.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <thread>

struct RegtestingSetup : public TestingSetup {
    RegtestingSetup() : TestingSetup(CBaseChainParams::REGTEST) {}
};

static const std::vector<unsigned char> V_OP_TRUE{0x00, OP_TRUE};

BOOST_FIXTURE_TEST_SUITE(validation_block_tests, RegtestingSetup)

struct TestSubscriber : public CValidationInterface {
    uint256 m_expected_tip;

    explicit TestSubscriber(uint256 tip) : m_expected_tip(tip) {}

    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, pindexNew->GetBlockHash());
    }

    void BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex, const std::vector<CTransactionRef>& txnConflicted) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, block->hashPrevBlock);
        BOOST_CHECK_EQUAL(m_expected_tip, pindex->pprev->GetBlockHash());

        m_expected_tip = block->GetHash();
    }

    void BlockDisconnected(const std::shared_ptr<const CBlock>& block) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, block->GetHash());

        m_expected_tip = block->hashPrevBlock;
    }
};

std::shared_ptr<CBlock> Block(const uint256& prev_hash, int height, BlockFinalTxEntry& entry)
{
    static int i = 0;
    static uint64_t time = Params().GenesisBlock().nTime;

    CScript pubKey;
    pubKey << i++ << OP_TRUE;

    auto ptemplate = BlockAssembler(Params()).CreateNewBlock(pubKey);
    auto pblock = std::make_shared<CBlock>(ptemplate->block);
    pblock->hashPrevBlock = prev_hash;
    pblock->nTime = ++time;

    pubKey.clear();
    {
        WitnessV0LongHash witness_program;
        CHash256().Write(&V_OP_TRUE[0], V_OP_TRUE.size()).Finalize(witness_program.begin());
        pubKey << OP_0 << ToByteVector(witness_program);
    }

    // Make the coinbase transaction with two outputs:
    // One zero-value one that has a unique pubkey to make sure that blocks at the same height can have a different hash
    // Another one that has the coinbase reward in a P2WSH with OP_TRUE as witness program to make it easy to spend
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    const CScript op_true = CScript() << OP_TRUE;
    bool initial_block_final = (txCoinbase.vout[0].scriptPubKey == op_true);
    txCoinbase.vout.resize(2 + initial_block_final + entry.IsNull());
    txCoinbase.vout[1 + initial_block_final].scriptPubKey = pubKey;
    txCoinbase.vout[1 + initial_block_final].nValue = txCoinbase.vout[0 + initial_block_final].nValue;
    txCoinbase.vout[0 + initial_block_final].nValue = 0;
    if (entry.IsNull()) {
        txCoinbase.vout.back().nValue = 0;
        txCoinbase.vout.back().scriptPubKey = CScript() << OP_TRUE;
    }
    txCoinbase.vin[0].scriptWitness.SetNull();
    txCoinbase.lock_height = (uint32_t)height;
    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));

    // Add block-final transaction
    if (ptemplate->has_block_final_tx) {
        pblock->vtx.pop_back();
    }
    if (!entry.IsNull()) {
        // Create transaction
        CMutableTransaction final_tx;
        final_tx.nVersion = 2;
        for (uint32_t n = 0; n < entry.size; ++n) {
            final_tx.vin.emplace_back(COutPoint(entry.hash, n));
        }
        final_tx.vout.emplace_back(0, CScript() << OP_TRUE);
        final_tx.lock_height = (uint32_t)height;
        // Add it to the block
        pblock->vtx.push_back(MakeTransactionRef(std::move(final_tx)));
    }

    return pblock;
}

std::shared_ptr<CBlock> FinalizeBlock(std::shared_ptr<CBlock> pblock, BlockFinalTxEntry& entry)
{
    LOCK(cs_main); // For LookupBlockIndex
    GenerateCoinbaseCommitment(*pblock, LookupBlockIndex(pblock->hashPrevBlock), Params().GetConsensus());
    // Store block-final info for next block
    if (!entry.IsNull()) {
        entry.hash = pblock->vtx.back()->GetHash();
        entry.size = 1;
    }

    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

    if (!pblock->m_aux_pow.IsNull()) {
        std::vector<unsigned char> extranonce;
        IncrementExtraNonceAux(*pblock, extranonce);
        while (!CheckAuxiliaryProofOfWork(*pblock, Params().GetConsensus())) {
            ++(pblock->m_aux_pow.m_aux_nonce);
        }
        const auto aux_hash2 = pblock->GetAuxiliaryHash(Params().GetConsensus()).second;
        CMutableTransaction txCoinbase(*pblock->vtx[0]);
        txCoinbase.vin[0].scriptSig.insert(txCoinbase.vin[0].scriptSig.end(), aux_hash2.begin(), aux_hash2.end());
        assert(txCoinbase.vin[0].scriptSig.size() <= 100);
        pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    }

    while (!CheckProofOfWork(*pblock, Params().GetConsensus())) {
        ++(pblock->nNonce);
    }

    return pblock;
}

// construct a valid block
std::shared_ptr<const CBlock> GoodBlock(const uint256& prev_hash, int height, BlockFinalTxEntry& entry)
{
    return FinalizeBlock(Block(prev_hash, height, entry), entry);
}

// construct an invalid block (but with a valid header)
std::shared_ptr<const CBlock> BadBlock(const uint256& prev_hash, int height, BlockFinalTxEntry& entry)
{
    auto pblock = Block(prev_hash, height, entry);

    CMutableTransaction coinbase_spend;
    coinbase_spend.vin.push_back(CTxIn(COutPoint(pblock->vtx[0]->GetHash(), 0), CScript(), 0));
    coinbase_spend.vout.push_back(pblock->vtx[0]->vout[0]);
    coinbase_spend.lock_height = pblock->vtx[0]->lock_height;

    CTransactionRef tx = MakeTransactionRef(coinbase_spend);
    pblock->vtx.insert(pblock->vtx.end() - !entry.IsNull(), tx);

    auto ret = FinalizeBlock(pblock, entry);
    return ret;
}

static BlockFinalTxEntry InitialBlockFinalTxEntry(const CBlock& block) {
    BlockFinalTxEntry entry;
    entry.hash = block.vtx[0]->GetHash();
    for (entry.size = 1; entry.size <= block.vtx[0]->vout.size(); ++entry.size) {
        if (!IsTriviallySpendable(*block.vtx[0], entry.size - 1, MANDATORY_SCRIPT_VERIFY_FLAGS | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_CLEANSTACK)) {
            break;
        }
    }
    return entry;
}

void BuildChain(const uint256& root, int root_height, const BlockFinalTxEntry& entry, int remaining, const unsigned int invalid_rate, const unsigned int branch_rate, const unsigned int max_size, std::vector<std::pair<std::shared_ptr<const CBlock>, bool>>& blocks)
{
    if (remaining <= 0 || blocks.size() >= max_size) return;
    int height = root_height + 1;

    bool gen_invalid = InsecureRandRange(100) < invalid_rate;
    bool gen_fork = InsecureRandRange(100) < branch_rate;

    BlockFinalTxEntry dummy, next_entry = entry;
    const std::shared_ptr<const CBlock> pblock = gen_invalid ? BadBlock(root, height, height > 100 ? next_entry : dummy) : GoodBlock(root, height, height > 100 ? next_entry : dummy);
    blocks.push_back(std::make_pair(pblock, !gen_invalid));
    if (!gen_invalid) {
        if (height == 1) {
            next_entry = InitialBlockFinalTxEntry(*pblock);
        }
        BuildChain(pblock->GetHash(), height, next_entry, remaining - 1, invalid_rate, branch_rate, max_size, blocks);
    }

    if (gen_fork) {
        next_entry = entry;
        blocks.push_back(std::make_pair(GoodBlock(root, height, height > 100 ? next_entry : dummy), true));
        if (height == 1) {
            next_entry = InitialBlockFinalTxEntry(*pblock);
        }
        BuildChain(blocks.back().first->GetHash(), height, next_entry, remaining - 1, invalid_rate, branch_rate, max_size, blocks);
    }
}

BOOST_AUTO_TEST_CASE(processnewblock_signals_ordering)
{
    // build a large-ish chain that's likely to have some forks
    std::vector<std::pair<std::shared_ptr<const CBlock>, bool>> blocks;
    while (blocks.size() < 50) {
        blocks.clear();
        BuildChain(Params().GenesisBlock().GetHash(), 0, BlockFinalTxEntry(), 100, 15, 10, 500, blocks);
    }

    bool ignored;
    CValidationState state;
    std::vector<CBlockHeader> headers;
    std::transform(blocks.begin(), blocks.end(), std::back_inserter(headers), [](std::pair<std::shared_ptr<const CBlock>, bool> b) { return b.first->GetBlockHeader(); });

    // Process all the headers so we understand the toplogy of the chain
    BOOST_CHECK(ProcessNewBlockHeaders(headers, state, Params()));

    // Connect the genesis block and drain any outstanding events
    BOOST_CHECK(ProcessNewBlock(Params(), std::make_shared<CBlock>(Params().GenesisBlock()), true, &ignored));
    SyncWithValidationInterfaceQueue();

    // subscribe to events (this subscriber will validate event ordering)
    const CBlockIndex* initial_tip = nullptr;
    {
        LOCK(cs_main);
        initial_tip = ::ChainActive().Tip();
    }
    TestSubscriber sub(initial_tip->GetBlockHash());
    RegisterValidationInterface(&sub);

    // create a bunch of threads that repeatedly process a block generated above at random
    // this will create parallelism and randomness inside validation - the ValidationInterface
    // will subscribe to events generated during block validation and assert on ordering invariance
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&blocks]() {
            bool ignored;
            FastRandomContext insecure;
            for (int i = 0; i < 1000; i++) {
                auto block = blocks[insecure.randrange(blocks.size() - 1)].first;
                ProcessNewBlock(Params(), block, true, &ignored);
            }

            // to make sure that eventually we process the full chain - do it here
            for (auto block : blocks) {
                if (block.second) {
                    bool processed = ProcessNewBlock(Params(), block.first, true, &ignored);
                    assert(processed);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
    while (GetMainSignals().CallbacksPending() > 0) {
        MilliSleep(100);
    }

    UnregisterValidationInterface(&sub);

    LOCK(cs_main);
    BOOST_CHECK_EQUAL(sub.m_expected_tip, ::ChainActive().Tip()->GetBlockHash());
}

/**
 * Test that mempool updates happen atomically with reorgs.
 *
 * This prevents RPC clients, among others, from retrieving immediately-out-of-date mempool data
 * during large reorgs.
 *
 * The test verifies this by creating a chain of `num_txs` blocks, matures their coinbases, and then
 * submits txns spending from their coinbase to the mempool. A fork chain is then processed,
 * invalidating the txns and evicting them from the mempool.
 *
 * We verify that the mempool updates atomically by polling it continuously
 * from another thread during the reorg and checking that its size only changes
 * once. The size changing exactly once indicates that the polling thread's
 * view of the mempool is either consistent with the chain state before reorg,
 * or consistent with the chain state after the reorg, and not just consistent
 * with some intermediate state during the reorg.
 */
BOOST_AUTO_TEST_CASE(mempool_locks_reorg)
{
    bool ignored;
    auto ProcessBlock = [&ignored](std::shared_ptr<const CBlock> block) -> bool {
        return ProcessNewBlock(Params(), block, /* fForceProcessing */ true, /* fNewBlock */ &ignored);
    };

    // Process all mined blocks
    BOOST_REQUIRE(ProcessBlock(std::make_shared<CBlock>(Params().GenesisBlock())));
    int height = 1;
    BlockFinalTxEntry dummy;
    auto last_mined = GoodBlock(Params().GenesisBlock().GetHash(), height, dummy);
    BOOST_REQUIRE(ProcessBlock(last_mined));
    ++height;
    // Record the initial block-final output.
    BlockFinalTxEntry entry;
    entry.hash = last_mined->vtx[0]->GetHash();
    entry.size = 1;

    // Run the test multiple times
    for (int test_runs = 3; test_runs > 0; --test_runs) {
        BOOST_CHECK_EQUAL(last_mined->GetHash(), ::ChainActive().Tip()->GetBlockHash());
        BOOST_CHECK_EQUAL(height, ::ChainActive().Tip()->nHeight + 1);

        // Later on split from here
        const uint256 split_hash{last_mined->GetHash()};
        const BlockFinalTxEntry split_entry = entry;
        const int split_height = height;

        // The first block contains the initial block-final output, which
        // makes the coinbase outputs offset.  Let's mine another block to
        // use as our "first" block instead.
        last_mined = GoodBlock(last_mined->GetHash(), height, height > 100 ? entry : dummy);
        BOOST_REQUIRE(ProcessBlock(last_mined));
        ++height;

        // Create a bunch of transactions to spend the miner rewards of the
        // most recent blocks
        std::vector<CTransactionRef> txs;
        for (int num_txs = 22; num_txs > 0; --num_txs) {
            CMutableTransaction mtx;
            mtx.vin.push_back(CTxIn{COutPoint{last_mined->vtx[0]->GetHash(), 1}, CScript{}});
            mtx.vin[0].scriptWitness.stack.push_back(V_OP_TRUE);
            mtx.vin[0].scriptWitness.stack.emplace_back();
            mtx.vout.push_back(last_mined->vtx[0]->vout[1]);
            mtx.vout[0].nValue -= 1000;
            mtx.lock_height = last_mined->vtx[0]->lock_height;
            txs.push_back(MakeTransactionRef(mtx));

            last_mined = GoodBlock(last_mined->GetHash(), height, height > 100 ? entry : dummy);
            BOOST_REQUIRE(ProcessBlock(last_mined));
            ++height;
        }

        // Mature the inputs of the txs
        for (int j = COINBASE_MATURITY; j > 0; --j) {
            last_mined = GoodBlock(last_mined->GetHash(), height, height > 100 ? entry : dummy);
            BOOST_REQUIRE(ProcessBlock(last_mined));
            ++height;
        }

        // Mine a reorg (and hold it back) before adding the txs to the mempool
        const uint256 tip_init{last_mined->GetHash()};

        std::vector<std::shared_ptr<const CBlock>> reorg;
        entry = split_entry;
        height = split_height;
        last_mined = GoodBlock(split_hash, height, height > 100 ? entry : dummy);
        reorg.push_back(last_mined);
        ++height;
        for (size_t j = COINBASE_MATURITY + txs.size() + 1; j > 0; --j) {
            last_mined = GoodBlock(last_mined->GetHash(), height, height > 100 ? entry : dummy);
            reorg.push_back(last_mined);
            ++height;
        }

        // Add the txs to the tx pool
        {
            LOCK(cs_main);
            CValidationState state;
            std::list<CTransactionRef> plTxnReplaced;
            for (const auto& tx : txs) {
                BOOST_REQUIRE(AcceptToMemoryPool(
                    ::mempool,
                    state,
                    tx,
                    /* pfMissingInputs */ &ignored,
                    &plTxnReplaced,
                    /* bypass_limits */ false,
                    /* nAbsurdFee */ 0));
            }
        }

        // Check that all txs are in the pool
        {
            LOCK(::mempool.cs);
            BOOST_CHECK_EQUAL(::mempool.mapTx.size(), txs.size());
        }

        // Run a thread that simulates an RPC caller that is polling while
        // validation is doing a reorg
        std::thread rpc_thread{[&]() {
            // This thread is checking that the mempool either contains all of
            // the transactions invalidated by the reorg, or none of them, and
            // not some intermediate amount.
            while (true) {
                LOCK(::mempool.cs);
                if (::mempool.mapTx.size() == 0) {
                    // We are done with the reorg
                    break;
                }
                // Internally, we might be in the middle of the reorg, but
                // externally the reorg to the most-proof-of-work chain should
                // be atomic. So the caller assumes that the returned mempool
                // is consistent. That is, it has all txs that were there
                // before the reorg.
                assert(::mempool.mapTx.size() == txs.size());
                continue;
            }
            LOCK(cs_main);
            // We are done with the reorg, so the tip must have changed
            assert(tip_init != ::ChainActive().Tip()->GetBlockHash());
        }};

        // Submit the reorg in this thread to invalidate and remove the txs from the tx pool
        for (const auto& b : reorg) {
            ProcessBlock(b);
        }
        // Check that the reorg was eventually successful
        BOOST_CHECK_EQUAL(last_mined->GetHash(), ::ChainActive().Tip()->GetBlockHash());

        // We can join the other thread, which returns when the reorg was successful
        rpc_thread.join();
    }
}
BOOST_AUTO_TEST_SUITE_END()
