// Copyright (c) 2020-2022 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <base58.h>
#include <consensus/tx_verify.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <node/context.h>
#include <node/miner.h>
#include <script/sign.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/miner.h>
#include <wallet/rpc/util.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>

#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

using node::NodeContext;
using node::CBlockTemplate;
using node::UpdateBlockFinalTxCommitment;

namespace wallet {

static std::shared_ptr<CWallet> GetWalletForBlockFinalTx(const NodeContext& node)
{
    if (!node.wallet_loader) {
        return nullptr;
    }
    WalletContext* context = node.wallet_loader->context();
    if (!context) {
        return nullptr;
    }
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets(*context);
    if (wallets.empty()) {
        return nullptr;
    }
    // The user can configure which wallet to use for the block-final tx inputs
    // with the '-walletblockfinaltx' option.  By default, the default wallet is
    // used.
    const std::string requestedwallet = gArgs.GetArg("-walletblockfinaltx", "");
    std::shared_ptr<CWallet> ret = GetWallet(*context, requestedwallet);
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
    return !wallets.empty() ? wallets[0] : nullptr;
}

bool AddBlockFinalTransaction(const NodeContext& node, CChainState& chainstate, CBlockTemplate& tmpl)
{
    if (tmpl.has_block_final_tx) {
        return true;
    }

    if (!gArgs.GetBoolArg("-walletblockfinaltx", true)) {
        // User has requested that block-final transactions only be present if
        // the block-final rule change has activated, so the wallet will not be
        // used to generate a block-final transaction prior to activation.
        return false;
    }

    std::shared_ptr<CWallet> pwallet = GetWalletForBlockFinalTx(node);
    if (!pwallet) {
        LogPrintf("No wallet; unable to fetch outputs for block-final transaction.\n");
        return false;
    }

    // Create block-final tx
    CMutableTransaction txFinal;
    txFinal.nVersion = 2;

    CTxDestination minesweep = DecodeDestination(gArgs.GetArg("-minesweepto", ""));
    CTxDestination carryforward = DecodeDestination(gArgs.GetArg("-carryforward", ""));

    // Get a vector of available outputs from the wallet.  This should not
    // include any outputs spent in this block, because outputs in the mempool
    // are excluded (and the transactions of the block were pulled from the
    // mempool).
    std::vector<COutput> outputs;
    {
        LOCK(pwallet->cs_wallet);
        AvailableCoins(*pwallet, outputs);
    }
    if (outputs.empty()) {
        LogPrintf("No available wallet outputs for block-final transaction.\n");
        return false;
    }

    // Index the outputs by txid:n
    std::map<COutPoint, COutput*> indexed;
    for (COutput& out : outputs) {
        indexed[{out.tx->GetHash(), static_cast<uint32_t>(out.i)}] = &out;
    }

    // Remove outputs spent in the block
    for (const auto& tx : tmpl.block.vtx) {
        for (const auto& txin : tx->vin) {
            if (indexed.count(txin.prevout)) {
                indexed.erase(txin.prevout);
            }
        }
    }

    // Gather inputs
    CAmount totalin = 0;
    for (const auto& item : indexed) {
        const COutPoint& prevout = item.first;
        const COutput& out = *item.second;
        if (!out.tx || out.nDepth<=0) {
            // Do not use unconfirmed outputs because we don't know if they will
            // be included in the block or not.
            continue;
        }
        txFinal.vin.emplace_back(prevout);
        totalin += out.tx->tx->vout[out.i].nValue;
        if (!IsValidDestination(minesweep)) {
            // If we're not sweeping the wallet, then we only need one.
            break;
        }
    }
    // Optional: sweep outputs
    if (IsValidDestination(minesweep)) {
        // The block-final transaction already includes all confirmed wallet
        // outputs.  So we just have to add an output to the minesweep address
        // claiming the funds.
        txFinal.vout.emplace_back(totalin, GetScriptForDestination(minesweep));
        totalin = 0;
    }
    // Optional: fixed carry-forward addr
    if (IsValidDestination(carryforward)) {
        // If a carry-forward address is specified, we make sure the tx includes
        // an output to that address, to enable future blocks to be mined.
        txFinal.vout.emplace_back(totalin, GetScriptForDestination(carryforward));
        totalin = 0;
    }
    // Default: send funds to reserve address
    if (totalin || !IsValidDestination(carryforward)) {
        LOCK(pwallet->cs_wallet);
        ReserveDestination reservedest(pwallet.get(), OutputType::BECH32);
        CTxDestination dest;
        bilingual_str error;
        if (!reservedest.GetReservedDestination(dest, true, error)) {
            LogPrintf("Keypool ran out while reserving script for block-final transaction, please call keypoolrefill: %s\n", error.translated);
            return false;
        }
        txFinal.vout.emplace_back(totalin, GetScriptForDestination(dest));
        totalin = 0;
        reservedest.KeepDestination();
    }

    // We should have input(s) for the block-final transaction from the wallet.
    // If not, we cannot create a block-final transaction because any
    // non-coinbase transaction must have valid inputs.
    if (txFinal.vin.empty()) {
        LogPrintf("Unable to create block-final transaction due to lack of inputs.\n");
        return false;
    }

    // Add commitment and sign block-final transaction.
    UpdateBlockFinalTxCommitment(txFinal, {});
    if (!SignBlockFinalTransaction(node, txFinal)) {
        LogPrintf("Error signing block-final transaction; cannot use invalid transaction.\n");
        return false;
    }

    // Add block-final transaction to block template.
    tmpl.block.vtx.emplace_back(MakeTransactionRef(std::move(txFinal)));
    // There are no fees in a wallet-generated block-final transaction.
    tmpl.vTxFees.push_back(0);
    // There might be a sigop cost though.
    tmpl.vTxSigOpsCost.push_back(GetTransactionSigOpCost(*tmpl.block.vtx.back(), chainstate.CoinsTip(), STANDARD_SCRIPT_VERIFY_FLAGS));

    int64_t weight = GetBlockWeight(tmpl.block);
    int64_t sigops = std::accumulate(tmpl.vTxSigOpsCost.begin(),
                                     tmpl.vTxSigOpsCost.end(), 0);
    while ((weight > MAX_BLOCK_WEIGHT) || (sigops > MAX_BLOCK_SIGOPS_COST)) {
        if (tmpl.block.vtx.size() <= 2) {
            // This should absolutely never happen in real circumstances, since
            // it would imply that the coinbase + block-final transaction are
            // too large to fit in a block.  But just in case, we handle it by
            // dropping the block-final transaction.
            LogPrintf("Coinbase + wallet block-final transaction exceed aggregate block limits (weight: %d, sigops: %d); removing block-final transaction from block template.\n", weight, sigops);
            tmpl.block.vtx.pop_back();
            tmpl.vTxFees.pop_back();
            tmpl.vTxSigOpsCost.pop_back();
            return false;
        }
        weight -= GetTransactionWeight(*tmpl.block.vtx[tmpl.block.vtx.size()-2]);
        sigops -= tmpl.vTxSigOpsCost[tmpl.vTxSigOpsCost.size()-2];
    }

    // Since we've added a block-final transaction created via the wallet, we
    // need to update the template to indicate that the block-final transaction
    // is present.
    tmpl.has_block_final_tx = true;

    return tmpl.has_block_final_tx;
}

bool SignBlockFinalTransaction(const NodeContext& node, CMutableTransaction &ret)
{
    CMutableTransaction mtx(ret);

    std::shared_ptr<CWallet> pwallet = GetWalletForBlockFinalTx(node);
    if (pwallet) {
        // Sign transaction
        std::map<COutPoint, Coin> coins;
        for (const CTxIn& txin : mtx.vin) {
            coins[txin.prevout]; // Create empty mpa entry keyed by prevout.
        }
        pwallet->chain().findCoins(coins);
        // Script verification errors
        std::map<int, bilingual_str> input_errors;
        if (!pwallet->SignTransaction(mtx, coins, SIGHASH_ALL, input_errors)) {
            LogPrintf("error signing block-final transaction with wallet \"%s\%", pwallet->GetName());
            for (const auto& error : input_errors) {
                LogPrintf("error creating signature input %d to block-final transaction: %s", error.first, error.second.translated);
                return false;
            }
        }
    }

    ret = mtx;
    return true;
}

} // ::wallet

// End of File
