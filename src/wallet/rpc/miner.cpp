// Copyright (c) 2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/rpc/miner.h>

#include <node/context.h> // for NodeContext
#include <wallet/miner.h> // for GetWalletForMiner
#include <wallet/rpc/wallet.h>
#include <wallet/wallet.h>

namespace wallet {

std::vector<RPCResult> GetMiningWalletInfoDesc() {
    return GetWalletInfoDesc();
}

UniValue GetMiningWalletInfo(const node::NodeContext& node) {
    bilingual_str error;
    const std::shared_ptr<const CWallet> pwallet = GetWalletForMiner(node, error);
    if (!pwallet) {
        return UniValue::VNULL;
    }
    return GetWalletInfo(*pwallet);
}

} // namespace wallet

// End of File
