// Copyright (c) 2023 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
