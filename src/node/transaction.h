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

#ifndef BITCOIN_NODE_TRANSACTION_H
#define BITCOIN_NODE_TRANSACTION_H

#include <attributes.h>
#include <primitives/transaction.h>
#include <uint256.h>

enum class TransactionError {
    OK, //!< No error
    MISSING_INPUTS,
    ALREADY_IN_CHAIN,
    P2P_DISABLED,
    MEMPOOL_REJECTED,
    MEMPOOL_ERROR,
    INVALID_PSBT,
    PSBT_MISMATCH,
    SIGHASH_MISMATCH,
};

std::string TransactionErrorString(const TransactionError error);

/**
 * Broadcast a transaction
 *
 * @param[in]  tx the transaction to broadcast
 * @param[out] &txid the txid of the transaction, if successfully broadcast
 * @param[out] &err_string reference to std::string to fill with error string if available
 * @param[in]  highfee Reject txs with fees higher than this (if 0, accept any fee)
 * return error
 */
NODISCARD TransactionError BroadcastTransaction(CTransactionRef tx, uint256& txid, std::string& err_string, const CAmount& highfee);

#endif // BITCOIN_NODE_TRANSACTION_H
