// Copyright (c) 2017-2022 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#ifndef FREICOIN_RPC_RAWTRANSACTION_UTIL_H
#define FREICOIN_RPC_RAWTRANSACTION_UTIL_H

#include <addresstype.h>
#include <consensus/amount.h>
#include <map>
#include <string>
#include <optional>

struct bilingual_str;
class FillableSigningProvider;
class UniValue;
struct CMutableTransaction;
class Coin;
class COutPoint;
class SigningProvider;

/**
 * Sign a transaction with the given keystore and previous transactions
 *
 * @param  mtx           The transaction to-be-signed
 * @param  keystore      Temporary keystore containing signing keys
 * @param  coins         Map of unspent outputs
 * @param  hashType      The signature hash type
 * @param result         JSON object where signed transaction results accumulate
 */
void SignTransaction(CMutableTransaction& mtx, const SigningProvider* keystore, const std::map<COutPoint, Coin>& coins, const UniValue& hashType, UniValue& result);
void SignTransactionResultToJSON(CMutableTransaction& mtx, bool complete, const std::map<COutPoint, Coin>& coins, const std::map<int, bilingual_str>& input_errors, UniValue& result);

/**
  * Parse a prevtxs UniValue array and get the map of coins from it
  *
  * @param  prevTxsUnival Array of previous txns outputs that tx depends on but may not yet be in the block chain
  * @param  keystore      A pointer to the temporary keystore if there is one
  * @param  coins         Map of unspent outputs - coins in mempool and current chain UTXO set, may be extended by previous txns outputs after call
  */
void ParsePrevouts(const UniValue& prevTxsUnival, FillableSigningProvider* keystore, std::map<COutPoint, Coin>& coins);

/** Normalize univalue-represented inputs and add them to the transaction */
void AddInputs(CMutableTransaction& rawTx, const UniValue& inputs_in, bool rbf);

/** Normalize univalue-represented outputs */
UniValue NormalizeOutputs(const UniValue& outputs_in);

/** Parse normalized outputs into destination, amount tuples */
std::vector<std::pair<CTxDestination, CAmount>> ParseOutputs(const UniValue& outputs);

/** Normalize, parse, and add outputs to the transaction */
void AddOutputs(CMutableTransaction& rawTx, const UniValue& outputs_in);

/** Create a transaction from univalue parameters */
CMutableTransaction ConstructTransaction(const UniValue& inputs_in, const UniValue& outputs_in, const UniValue& locktime, const UniValue& lockheight, int current_height, std::optional<bool> rbf);

#endif // FREICOIN_RPC_RAWTRANSACTION_UTIL_H
