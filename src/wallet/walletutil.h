// Copyright (c) 2017-2018 The Bitcoin Core developers
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

#ifndef FREICOIN_WALLET_WALLETUTIL_H
#define FREICOIN_WALLET_WALLETUTIL_H

#include <fs.h>

#include <vector>

//! Get the path of the wallet directory.
fs::path GetWalletDir();

//! Get wallets in wallet directory.
std::vector<fs::path> ListWalletDir();

//! The WalletLocation class provides wallet information.
class WalletLocation final
{
    std::string m_name;
    fs::path m_path;

public:
    explicit WalletLocation() {}
    explicit WalletLocation(const std::string& name);

    //! Get wallet name.
    const std::string& GetName() const { return m_name; }

    //! Get wallet absolute path.
    const fs::path& GetPath() const { return m_path; }

    //! Return whether the wallet exists.
    bool Exists() const;
};

#endif // FREICOIN_WALLET_WALLETUTIL_H
