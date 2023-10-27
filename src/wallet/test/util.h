// Copyright (c) 2021 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
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

#ifndef BITCOIN_WALLET_TEST_UTIL_H
#define BITCOIN_WALLET_TEST_UTIL_H

#include <memory>

class ArgsManager;
class CChain;
class CKey;
namespace interfaces {
class Chain;
} // namespace interfaces

namespace wallet {
class CWallet;

std::unique_ptr<CWallet> CreateSyncedWallet(interfaces::Chain& chain, CChain& cchain, ArgsManager& args, const CKey& key);
} // namespace wallet

#endif // BITCOIN_WALLET_TEST_UTIL_H
