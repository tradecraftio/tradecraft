// Copyright (c) 2020-2022 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
