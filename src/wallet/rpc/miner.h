// Copyright (c) 2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
