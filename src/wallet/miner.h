// Copyright (c) 2020-2023 The Freicoin Developers
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
