// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_MINER_H
#define BITCOIN_WALLET_MINER_H

#include <node/context.h> // for node::NodeContext
#include <node/miner.h> // for node::CBlockTemplate
#include <primitives/transaction.h> // for CMutableTransaction
#include <script/standard.h> // for CTxDestination
#include <util/translation.h> // for bilingual_str
#include <validation.h> // for Chainstate

namespace wallet {

//! Use the wallet to add a block-final transaction to an existing block
//! template.  This involves first creating and signing a transaction using
//! wallet inputs, and then (possibly) removing transactions from the end of
//! the block to make room.  Return value indicates whether the template has a
//! block-final transaction after the call.
bool AddBlockFinalTransaction(const node::NodeContext& node, Chainstate& chainstate, node::CBlockTemplate& tmpl, bilingual_str& error);
//! Update the signature of a block-final transaction.
bool SignBlockFinalTransaction(const node::NodeContext& node, CMutableTransaction &ret, bilingual_str& error);
//! Release (un-cache) the wallet used for signing block-final transactions.
void ReleaseBlockFinalWallet();

//! Reserve a destination for mining.
bool ReserveMiningDestination(const node::NodeContext& node, CTxDestination& dest, bilingual_str& error);
//! Mark a destination as permanently used, due to a block being found.
bool KeepMiningDestination(const CTxDestination& dest);
//! Release any reserved destinations.
void ReleaseMiningDestinations();

} // namespace wallet

#endif // BITCOIN_WALLET_MINER_H

// End of File
