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

#ifndef BITCOIN_STRATUM_H
#define BITCOIN_STRATUM_H

#include <node/context.h>

/** The minimum difficulty for stratum mining clients.  A difficulty setting of 10^3 would take a 1Thps miner ~4 seconds to find a share. */
const double DEFAULT_MINING_DIFFICULTY = 1e3;

/** Configure the stratum server. */
bool InitStratumServer(node::NodeContext& node);

/** Interrupt the stratum server connections. */
void InterruptStratumServer();

/** Cleanup stratum server network connections and free resources. */
void StopStratumServer();

#endif // BITCOIN_STRATUM_H

// End of File
