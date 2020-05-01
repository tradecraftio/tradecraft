// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_H
#define BITCOIN_STRATUM_H

#include <node/context.h>

/** Configure the stratum server. */
bool InitStratumServer(node::NodeContext& node);

/** Interrupt the stratum server connections. */
void InterruptStratumServer();

/** Cleanup stratum server network connections and free resources. */
void StopStratumServer();

#endif // BITCOIN_STRATUM_H

// End of File
