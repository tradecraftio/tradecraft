// Copyright (c) 2019-2020 The Bitcoin Core developers
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

#ifndef BITCOIN_TEST_UTIL_TRANSACTION_UTILS_H
#define BITCOIN_TEST_UTIL_TRANSACTION_UTILS_H

#include <primitives/transaction.h>

#include <array>

class FillableSigningProvider;
class CCoinsViewCache;

// create crediting transaction
// [1 coinbase input => 1 output with given scriptPubkey and value]
CMutableTransaction BuildCreditingTransaction(const CScript& scriptPubKey, int nValue = 0);

// create spending transaction
// [1 input with referenced transaction outpoint, scriptSig, scriptWitness =>
//  1 output with empty scriptPubKey, full value of referenced transaction]
CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CScriptWitness& scriptWitness, const CTransaction& txCredit);

// Helper: create two dummy transactions, each with two outputs.
// The first has nValues[0] and nValues[1] outputs paid to a TxoutType::PUBKEY,
// the second nValues[2] and nValues[3] outputs paid to a TxoutType::PUBKEYHASH.
std::vector<CMutableTransaction> SetupDummyInputs(FillableSigningProvider& keystoreRet, CCoinsViewCache& coinsRet, const std::array<CAmount,4>& nValues);

#endif // BITCOIN_TEST_UTIL_TRANSACTION_UTILS_H
