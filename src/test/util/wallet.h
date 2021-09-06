// Copyright (c) 2019 The Bitcoin Core developers
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

#ifndef FREICOIN_TEST_UTIL_WALLET_H
#define FREICOIN_TEST_UTIL_WALLET_H

#include <string>

namespace wallet {
class CWallet;
} // namespace wallet

// Constants //

extern const std::string ADDRESS_FCRT1_UNSPENDABLE;

// RPC-like //

/** Import the address to the wallet */
void importaddress(wallet::CWallet& wallet, const std::string& address);
/** Returns a new address from the wallet */
std::string getnewaddress(wallet::CWallet& w);


#endif // FREICOIN_TEST_UTIL_WALLET_H
