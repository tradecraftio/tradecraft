// Copyright (c) 2019 The Bitcoin Core developers
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

#ifndef BITCOIN_TEST_UTIL_MINING_H
#define BITCOIN_TEST_UTIL_MINING_H

#include <memory>
#include <string>

class CBlock;
class CScript;
class CTxIn;
struct NodeContext;

/** Returns the generated coin */
CTxIn MineBlock(const NodeContext&, const CScript& coinbase_scriptPubKey);

/** Prepare a block to be mined */
std::shared_ptr<CBlock> PrepareBlock(const NodeContext&, const CScript& coinbase_scriptPubKey);

/** RPC-like helper function, returns the generated coin */
CTxIn generatetoaddress(const NodeContext&, const std::string& address);

#endif // BITCOIN_TEST_UTIL_MINING_H
