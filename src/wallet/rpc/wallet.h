// Copyright (c) 2016-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPC_WALLET_H
#define BITCOIN_WALLET_RPC_WALLET_H

#include <rpc/util.h>
#include <span.h>
#include <wallet/wallet.h>

class CRPCCommand;

namespace wallet {
std::vector<RPCResult> GetWalletInfoDesc();
UniValue GetWalletInfo(const CWallet& wallet);
Span<const CRPCCommand> GetWalletRPCCommands();
} // namespace wallet

#endif // BITCOIN_WALLET_RPC_WALLET_H
