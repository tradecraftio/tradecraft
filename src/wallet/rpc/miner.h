// Copyright (c) 2023 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BITCOIN_WALLET_RPC_MINER_H
#define BITCOIN_WALLET_RPC_MINER_H

#include <node/context.h> // for NodeContext
#include <rpc/util.h> // for RPCResult
#include <univalue.h> // for UniValue

namespace wallet {

//! Return specification for GetMiningWalletInfo().
std::vector<RPCResult> GetMiningWalletInfoDesc();

//! Get information about the wallet used for mining.
UniValue GetMiningWalletInfo(const node::NodeContext& node);

} // namespace wallet

#endif // BITCOIN_WALLET_RPC_MINER_H

// End of File
