// Copyright (c) 2019-2021 The Bitcoin Core developers
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

#ifndef BITCOIN_NODE_COIN_H
#define BITCOIN_NODE_COIN_H

#include <map>

class COutPoint;
class Coin;

namespace node {
struct NodeContext;

/**
 * Look up unspent output information. Returns coins in the mempool and in the
 * current chain UTXO set. Iterates through all the keys in the map and
 * populates the values.
 *
 * @param[in] node The node context to use for lookup
 * @param[in,out] coins map to fill
 */
void FindCoins(const node::NodeContext& node, std::map<COutPoint, Coin>& coins);
} // namespace node

#endif // BITCOIN_NODE_COIN_H
