// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
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

#ifndef BITCOIN_WALLET_LOAD_H
#define BITCOIN_WALLET_LOAD_H

#include <string>
#include <vector>

class CScheduler;

namespace interfaces {
class Chain;
} // namespace interfaces

//! Responsible for reading and validating the -wallet arguments and verifying the wallet database.
//! This function will perform salvage on the wallet if requested, as long as only one wallet is
//! being loaded (WalletInit::ParameterInteraction() forbids -salvagewallet, -zapwallettxes or -upgradewallet with multiwallet).
bool VerifyWallets(interfaces::Chain& chain, const std::vector<std::string>& wallet_files);

//! Load wallet databases.
bool LoadWallets(interfaces::Chain& chain, const std::vector<std::string>& wallet_files);

//! Complete startup of wallets.
void StartWallets(CScheduler& scheduler);

//! Flush all wallets in preparation for shutdown.
void FlushWallets();

//! Stop all wallets. Wallets will be flushed first.
void StopWallets();

//! Close all wallets.
void UnloadWallets();

#endif // BITCOIN_WALLET_LOAD_H
