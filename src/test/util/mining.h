// Copyright (c) 2019-2021 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <node/miner.h>

#include <memory>
#include <string>
#include <vector>

class CBlock;
class CChainParams;
class CScript;
class CTxIn;
namespace node {
struct NodeContext;
} // namespace node

/** Create a blockchain, starting from genesis */
std::vector<std::shared_ptr<CBlock>> CreateBlockChain(size_t total_height, const CChainParams& params);

/** Returns the generated coin */
CTxIn MineBlock(const node::NodeContext&, const CScript& coinbase_scriptPubKey);

/** Prepare a block to be mined */
std::shared_ptr<CBlock> PrepareBlock(const node::NodeContext&, const CScript& coinbase_scriptPubKey);
std::shared_ptr<CBlock> PrepareBlock(const node::NodeContext& node, const CScript& coinbase_scriptPubKey,
                                     const node::BlockAssembler::Options& assembler_options);

/** RPC-like helper function, returns the generated coin */
CTxIn generatetoaddress(const node::NodeContext&, const std::string& address);

#endif // BITCOIN_TEST_UTIL_MINING_H
