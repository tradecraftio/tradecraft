// Copyright (c) 2020-2023 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <wallet/miner.h>

#include <consensus/tx_verify.h> // for GetTransactionSigOpCost
#include <interfaces/wallet.h> // for WalletLoader
#include <key_io.h> // for DecodeDestination
#include <node/context.h> // for NodeContext
#include <node/miner.h> // for node::CBlockTemplate, node::UpdateBlockFinalTxCommitment
#include <primitives/transaction.h> // for CMutableTransaction
#include <script/standard.h> // for CTxDestination
#include <sync.h> // for RecursiveMutex
#include <threadsafety.h> // for EXCLUSIVE_LOCKS_REQUIRED, GUARDED_BY
#include <util/system.h> // for gArgs
#include <util/translation.h> // for bilingual_str, _("...")
#include <validation.h> // for Chainstate
#include <wallet/coinselection.h> // for COutput
#include <wallet/context.h> // for WalletContext
#include <wallet/spend.h> // for AvailableCoins
#include <wallet/wallet.h> // for CWallet, GetWallets, GetWallet, ReserveDestination

#include <memory> // for std::shared_ptr
#include <numeric> // for std::accumulate
#include <optional> // for std::optional
#include <string> // for std::string
#include <utility> // for std::move, std::pair
#include <vector> // for std::vector

using node::NodeContext;
using node::CBlockTemplate;
using node::UpdateBlockFinalTxCommitment;

namespace wallet {

//! Critical seciton guarding access to any of the stratum global state
static RecursiveMutex cs_block_final_wallet;

//! Cached pointer to the wallet used for block-final transactions.
static std::shared_ptr<CWallet> g_block_final_wallet GUARDED_BY(cs_block_final_wallet);

static std::shared_ptr<CWallet> GetWalletForBlockFinalTx(const NodeContext& node, bilingual_str& error) {
    // Must take lock before accessing static variables.
    LOCK(cs_block_final_wallet);
    // Use a global variable to cache the wallet pointer.  This is safe as
    // this whole function requires cs_block_final_wallet to be held.
    if (g_block_final_wallet) {
        // The wallet has already been configured.
        return g_block_final_wallet;
    }
    if (!node.wallet_loader) {
        error = _("The wallet subsystem is not enabled.");
        return nullptr;
    }
    WalletContext* context = node.wallet_loader->context();
    if (!context) {
        error = _("The wallet subsystem is not configured.");
        return nullptr;
    }
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets(*context);
    if (wallets.empty()) {
        error = _("No wallets available for mining.");
        return nullptr;
    }
    // The user can configure which wallet to use for when mining with the
    // '-walletblockfinaltx' option.  If the user does not specify a wallet,
    // then the first wallet (the default wallet) is used.
    const std::string requestedwallet = gArgs.GetArg("-walletblockfinaltx", "");
    g_block_final_wallet = GetWallet(*context, requestedwallet);
    if (g_block_final_wallet) {
        // Found it!
        return g_block_final_wallet;
    }
    // The user requested a wallet that is not loaded.  Fall back to the
    // default wallet, but report the error so the user can fix their
    // configuration.
    if (requestedwallet != "" && requestedwallet != "0") {
        LogPrintf("Requested wallet \"%s\" be used for the stratum mining service, but no such wallet found.\n", requestedwallet);
    }
    // The user can disable generating block-final transactions using wallet
    // inputs by setting '-walletblockfinaltx' to 0 or false, or or
    // '-nowalletblockfinaltx=1'.
    if (!gArgs.GetBoolArg("-walletblockfinaltx", true)) {
        error = _("Using wallet inputs for block-final transaction is disabled.");
        return nullptr;
    }
    // If we get this far, it is because the default wallet was requested.
    if (!wallets.empty()) {
        // The default wallet is the first wallet.
        g_block_final_wallet = wallets[0];
    }
    return g_block_final_wallet;
}

bool AddBlockFinalTransaction(const NodeContext& node, Chainstate& chainstate, CBlockTemplate& tmpl, bilingual_str& error) {
    // If the block template already has a block-final transaction, (e.g.
    // because the finaltx soft-fork has activated, or because we've been
    // called twice), then there is nothing to do.
    if (tmpl.has_block_final_tx) {
        return true;
    }
    if (!gArgs.GetBoolArg("-walletblockfinaltx", true)) {
        // User has requested that block-final transactions only be present if
        // the finaltx soft-fork has activated, so the wallet will not be used
        // to generate a block-final transaction prior to activation.
        return false;
    }

    // Get the wallet to use for block-final transactions.
    std::shared_ptr<CWallet> pwallet = GetWalletForBlockFinalTx(node, error);
    if (!pwallet) {
        // The user has requested that block-final transactions be generated
        // using wallet inputs, but no wallet is available.
        LogPrintf("No wallet; unable to fetch outputs for block-final transaction: %s\n", error.translated);
        return false;
    }

    // Create block-final tx
    CMutableTransaction txFinal;
    txFinal.nVersion = 2;

    // Fetch minesweep and carryforward addresses from configuration options.
    CTxDestination minesweep = DecodeDestination(gArgs.GetArg("-minesweepto", ""));
    CTxDestination carryforward = DecodeDestination(gArgs.GetArg("-carryforward", ""));

    // Get a vector of available outputs from the wallet.  This should not
    // include any outputs spent in this block, because outputs in the mempool
    // are excluded (and the transactions of the block were pulled from the
    // mempool).
    std::vector<COutput> outputs;
    {
        LOCK(pwallet->cs_wallet);
        outputs = AvailableCoins(*pwallet).All();
    }
    if (outputs.empty()) {
        LogPrintf("No available wallet outputs for block-final transaction.\n");
        return false;
    }

    // Index the outputs by txid:n
    std::map<COutPoint, const COutput*> indexed;
    for (const COutput& out : outputs) {
        indexed[out.outpoint] = &out;
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
        if (!out.spendable || out.depth<=0) {
            // Do not use unconfirmed outputs because we haven't checked if
            // they were included in the block or not.
            continue;
        }
        txFinal.vin.emplace_back(prevout);
        totalin += out.txout.nValue;
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
        // If a carry-forward address is specified, we make sure the tx
        // includes an output to that address, to enable future blocks to be
        // mined.
        txFinal.vout.emplace_back(totalin, GetScriptForDestination(carryforward));
        totalin = 0;
    }
    // Default: send funds to reserve address
    if (totalin || !IsValidDestination(carryforward)) {
        LOCK(pwallet->cs_wallet);
        ReserveDestination reservedest(pwallet.get(), OutputType::BECH32);
        auto dest = reservedest.GetReservedDestination(true);
        if (!dest) {
            LogPrintf("Keypool ran out while reserving script for block-final transaction, please call keypoolrefill: %s\n", ErrorString(dest).translated);
            return false;
        }
        txFinal.vout.emplace_back(totalin, GetScriptForDestination(*dest));
        totalin = 0;
        reservedest.KeepDestination();
    }

    // We should have input(s) for the block-final transaction from the
    // wallet.  If not, we cannot create a block-final transaction because
    // any non-coinbase transaction must have valid inputs.
    if (txFinal.vin.empty()) {
        LogPrintf("Unable to create block-final transaction due to lack of inputs.\n");
        return false;
    }

    // Add commitment and sign block-final transaction.
    UpdateBlockFinalTxCommitment(txFinal, {});
    if (!SignBlockFinalTransaction(node, txFinal, error)) {
        LogPrintf("Error signing block-final transaction; cannot use invalid transaction: %s\n", error.translated);
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
            // This should absolutely never happen in real circumstances,
            // since it would imply that the coinbase + block-final
            // transaction are too large to fit in a block.  But just in case,
            // we handle it by dropping the block-final transaction.
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
    // need to update the template to indicate that the block-final
    // transaction is present.
    tmpl.has_block_final_tx = true;

    return tmpl.has_block_final_tx;
}

bool SignBlockFinalTransaction(const NodeContext& node, CMutableTransaction &ret, bilingual_str& error) {
    CMutableTransaction mtx(ret);

    std::shared_ptr<CWallet> pwallet = GetWalletForBlockFinalTx(node, error);
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
            LogPrintf("error signing block-final transaction with wallet \"%s\"", pwallet->GetName());
            for (const auto& error : input_errors) {
                LogPrintf("error creating signature input %d to block-final transaction: %s", error.first, error.second.translated);
                return false;
            }
        }
    }

    ret = mtx;
    return true;
}

void ReleaseBlockFinalWallet() {
    LOCK(cs_block_final_wallet);
    g_block_final_wallet.reset();
}

//! Critical seciton guarding access to any of the stratum global state
static RecursiveMutex cs_stratum_wallet;

//! The wallet used to create mining destinations.
static std::shared_ptr<CWallet> g_stratum_wallet GUARDED_BY(cs_stratum_wallet);

static std::shared_ptr<CWallet> GetWalletForMiner(const NodeContext& node, bilingual_str& error) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum_wallet) {
    // Use a global variable to cache the wallet pointer.  This is safe this
    // whole function requires cs_stratum_wallet to be held.
    if (g_stratum_wallet) {
        // The wallet has already been configured.
        return g_stratum_wallet;
    }
    if (!node.wallet_loader) {
        error = _("The wallet subsystem is not enabled.");
        return nullptr;
    }
    WalletContext* context = node.wallet_loader->context();
    if (!context) {
        error = _("The wallet subsystem is not configured.");
        return nullptr;
    }
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets(*context);
    if (wallets.empty()) {
        error = _("No wallets available for mining.");
        return nullptr;
    }
    // The user can configure which wallet to use for when mining with the
    // '-stratumwallet' option.  If the user does not specify a wallet, then
    // the first wallet (the default wallet) is used.
    const std::string requestedwallet = gArgs.GetArg("-stratumwallet", "");
    g_stratum_wallet = GetWallet(*context, requestedwallet);
    if (g_stratum_wallet) {
        // Found it!
        return g_stratum_wallet;
    }
    // The user requested a wallet that is not loaded.  Fall back to the default
    // wallet, but report the error so the user can fix their configuration.
    if (requestedwallet != "" && requestedwallet != "0") {
        LogPrintf("Requested wallet \"%s\" be used for the stratum mining service, but no such wallet found.\n", requestedwallet);
    }
    // The user can disable direct mining to an internal wallet by setting
    // '-stratumwallet' to 0 or false, or or '-nostratumwallet=1'.
    if (!gArgs.GetBoolArg("-stratumwallet", true)) {
        error = _("Direct mining to an internal wallet is disabled.");
        return nullptr;
    }
    // If we get this far, it is because the default wallet was requested.
    if (!wallets.empty()) {
        // The default wallet is the first wallet.
        g_stratum_wallet = wallets[0];
    }
    return g_stratum_wallet;
}

//! The destination to use for stratum mining
static std::optional<ReserveDestination> g_next_reservation GUARDED_BY(cs_stratum_wallet);
static std::optional<CTxDestination> g_next_destination GUARDED_BY(cs_stratum_wallet);

bool ReserveMiningDestination(const NodeContext& node, CTxDestination& dest, bilingual_str& error) {
    LOCK(cs_stratum_wallet);
    // Get the next destination to use for mining.
    if (!g_next_reservation.has_value()) {
        // Get the wallet to use for mining.
        std::shared_ptr<CWallet> pwallet = GetWalletForMiner(node, error);
        if (!pwallet) {
            return false;
        }
        // Reserve the new destination.
        g_next_reservation.reset();
        g_next_reservation.emplace(pwallet.get(), OutputType::BECH32);
        // Export the ReserveDestination as a CTxDestination.
        auto op_dest = g_next_reservation->GetReservedDestination(true);
        if (!op_dest) {
            // Release the destination back to the pool.
            g_next_reservation.reset();
            return false;
        }
        dest = *op_dest;
        // Cache the destination value.
        g_next_destination.reset();
        g_next_destination = dest;
        return true;
    }
    // The destination is already reserved, so just return it.
    dest = *g_next_destination;
    return true;
}

//! Mark the destination as used, and therefore kept out of the pool.
bool KeepMiningDestination(const CTxDestination& dest) {
    LOCK(cs_stratum_wallet);
    if (g_next_destination.has_value() && dest == g_next_destination.value()) {
        // The destination is still in use, so mark it as permanently reserved
        // and get ready for the next destination.
        g_next_reservation->KeepDestination();
        g_next_reservation.reset();
        g_next_destination.reset();
        return true;
    }
    return false;
}

void ReleaseMiningDestinations() {
    LOCK(cs_stratum_wallet);
    // Release the destination back to the pool.
    if (g_next_reservation.has_value()) {
        g_next_reservation->ReturnDestination();
    }
    g_next_reservation.reset();
    g_next_destination.reset();
    g_stratum_wallet.reset();
}

} // namespace wallet

// End of File
