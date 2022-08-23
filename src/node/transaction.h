// Copyright (c) 2017-2019 The Bitcoin Core developers
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

#ifndef FREICOIN_NODE_TRANSACTION_H
#define FREICOIN_NODE_TRANSACTION_H

#include <attributes.h>
#include <primitives/transaction.h>
#include <util/error.h>

struct NodeContext;

/**
 * Submit a transaction to the mempool and (optionally) relay it to all P2P peers.
 *
 * Mempool submission can be synchronous (will await mempool entry notification
 * over the CValidationInterface) or asynchronous (will submit and not wait for
 * notification), depending on the value of wait_callback. wait_callback MUST
 * NOT be set while cs_main, cs_mempool or cs_wallet are held to avoid
 * deadlock.
 *
 * @param[in]  node reference to node context
 * @param[in]  tx the transaction to broadcast
 * @param[out] err_string reference to std::string to fill with error string if available
 * @param[in]  max_tx_fee reject txs with fees higher than this (if 0, accept any fee)
 * @param[in]  relay flag if both mempool insertion and p2p relay are requested
 * @param[in]  wait_callback wait until callbacks have been processed to avoid stale result due to a sequentially RPC.
 * return error
 */
NODISCARD TransactionError BroadcastTransaction(NodeContext& node, CTransactionRef tx, std::string& err_string, const CAmount& max_tx_fee, bool relay, bool wait_callback);

#endif // FREICOIN_NODE_TRANSACTION_H
