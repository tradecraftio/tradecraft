// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FREICOIN_WALLET_MINER_H
#define FREICOIN_WALLET_MINER_H

#include <node/context.h> // for node::NodeContext
#include <script/standard.h> // for CTxDestination
#include <util/translation.h> // for bilingual_str

namespace wallet {

//! Reserve a destination for mining.
bool ReserveMiningDestination(const node::NodeContext& node, CTxDestination& dest, bilingual_str& error);
//! Mark a destination as permanently used, due to a block being found.
bool KeepMiningDestination(const CTxDestination& dest);
//! Release any reserved destinations.
void ReleaseMiningDestinations();

} // namespace wallet

#endif // FREICOIN_WALLET_MINER_H

// End of File
