// Copyright (c) 2016 The Bitcoin Core developers
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

#ifndef BITCOIN_WALLET_WALLETTOOL_H
#define BITCOIN_WALLET_WALLETTOOL_H

#include <wallet/ismine.h>
#include <wallet/wallet.h>

namespace WalletTool {

std::shared_ptr<CWallet> CreateWallet(const std::string& name, const fs::path& path);
std::shared_ptr<CWallet> LoadWallet(const std::string& name, const fs::path& path);
void WalletShowInfo(CWallet* wallet_instance);
bool ExecuteWalletToolFunc(const std::string& command, const std::string& file);

} // namespace WalletTool

#endif // BITCOIN_WALLET_WALLETTOOL_H
