// Copyright (c) 2020-2022 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BITCOIN_WALLET_MINER_H
#define BITCOIN_WALLET_MINER_H

#include <node/context.h>
#include <node/miner.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <validation.h>

namespace wallet {

/** Use the wallet to add a block-final transaction to an existing block template.  This involves first creating and signing a transaction using wallet inputs, and then (possibly) removing transactions from the end of the block to make room.  Return value indicates whether the template has a block-final transaction after the call. */
bool AddBlockFinalTransaction(const node::NodeContext& node, CChainState& chainstate, node::CBlockTemplate& tmpl);
bool SignBlockFinalTransaction(const node::NodeContext& node, CMutableTransaction &ret);

} // ::wallet

#endif // BITCOIN_WALLET_MINER_H

// End of File
