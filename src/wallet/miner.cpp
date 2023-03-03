// Copyright (c) 2020-2023 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <wallet/miner.h>

#include <interfaces/wallet.h> // for WalletLoader
#include <node/context.h> // for NodeContext
#include <script/standard.h> // for CTxDestination
#include <sync.h> // for RecursiveMutex
#include <threadsafety.h> // for EXCLUSIVE_LOCKS_REQUIRED, GUARDED_BY
#include <util/system.h> // for gArgs
#include <util/translation.h> // for bilingual_str, _("...")
#include <wallet/context.h> // for WalletContext
#include <wallet/wallet.h> // for CWallet, GetWallets, GetWallet, ReserveDestination

#include <memory> // for std::shared_ptr
#include <optional> // for std::optional
#include <string> // for std::string
#include <utility> // for std::move, std::pair
#include <vector> // for std::vector

using node::NodeContext;

namespace wallet {

//! Critical seciton guarding access to any of the stratum global state
static RecursiveMutex cs_stratum_wallet;

static std::shared_ptr<CWallet> GetWalletForMiner(const NodeContext& node, bilingual_str& error) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum_wallet) {
    // Use a static variable to cache the wallet pointer.  This is safe this
    // whole function requires cs_stratum_wallet to be held.
    static std::shared_ptr<CWallet> pwallet;
    if (pwallet) {
        // The wallet has already been configured.
        return pwallet;
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
    pwallet = GetWallet(*context, requestedwallet);
    if (pwallet) {
        // Found it!
        return pwallet;
    }
    // The user requested a wallet that is not loaded.  Fall back to the default
    // wallet, but report the error so the user can fix their configuration.
    if (requestedwallet != "" && requestedwallet != "0") {
        LogPrintf("Requested wallet \"%s\" be used for the stratum mining service, but no such wallet found.\n", requestedwallet);
    }
    // The user can disable direct mining to an internal wallet by setting
    // '-stratumwallet' to 0 or false, or or '-nostratumwallet=1'.
    if (!gArgs.GetBoolArg("-walletblockfinaltx", true)) {
        error = _("Direct mining to an internal wallet is disabled.");
        return nullptr;
    }
    // If we get this far, it is because the default wallet was requested.
    if (!wallets.empty()) {
        // The default wallet is the first wallet.
        pwallet = wallets[0];
    }
    return pwallet;
}

//! The destination to use for stratum mining
static std::optional<ReserveDestination> g_next_reservation GUARDED_BY(cs_stratum_wallet);
static std::optional<CTxDestination> g_next_destination GUARDED_BY(cs_stratum_wallet);

bool ReserveMiningDestination(const NodeContext& node, CTxDestination& pubkey, bilingual_str& error) {
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
        if (!g_next_reservation->GetReservedDestination(pubkey, true, error)) {
            // Release the destination back to the pool.
            g_next_reservation.reset();
            return false;
        }
        // Cache the destination value.
        g_next_destination.reset();
        g_next_destination = pubkey;
        return true;
    }
    // The destination is already reserved, so just return it.
    pubkey = *g_next_destination;
    return true;
}

//! Mark the destination as used, and therefore kept out of the pool.
bool KeepMiningDestination(const CTxDestination& pubkey) {
    LOCK(cs_stratum_wallet);
    if (g_next_destination.has_value() && pubkey == g_next_destination.value()) {
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
}

} // namespace wallet

// End of File
