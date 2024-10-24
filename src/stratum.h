// Copyright (c) 2020-2024 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or <https://www.opensource.org/licenses/mit-license.php>.

#ifndef FREICOIN_STRATUM_H
#define FREICOIN_STRATUM_H

#include <node/context.h>

/** The minimum difficulty for stratum mining clients.  A difficulty setting of 10^3 would take a 1Thps miner ~4 seconds to find a share. */
const double DEFAULT_MINING_DIFFICULTY = 1e3;

/** Configure the stratum server. */
bool InitStratumServer(node::NodeContext& node);

/** Interrupt the stratum server connections. */
void InterruptStratumServer();

/** Cleanup stratum server network connections and free resources. */
void StopStratumServer();

#endif // FREICOIN_STRATUM_H

// End of File
