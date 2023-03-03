// Copyright (c) 2020-2023 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef FREICOIN_WALLET_MINER_H
#define FREICOIN_WALLET_MINER_H

#include <node/context.h> // for node::NodeContext
#include <script/standard.h> // for CTxDestination
#include <util/translation.h> // for bilingual_str

namespace wallet {

//! Reserve a destination for mining.
bool ReserveMiningDestination(const node::NodeContext& node, CTxDestination& pubkey, bilingual_str& error);
//! Mark a destination as permanently used, due to a block being found.
bool KeepMiningDestination(const CTxDestination& pubkey);
//! Release any reserved destinations.
void ReleaseMiningDestinations();

} // namespace wallet

#endif // FREICOIN_WALLET_MINER_H

// End of File
