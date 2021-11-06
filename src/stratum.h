// Copyright (c) 2020-2021 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef FREICOIN_STRATUM_H
#define FREICOIN_STRATUM_H

/** Configure the stratum server. */
bool InitStratumServer();

/** Interrupt the stratum server connections. */
void InterruptStratumServer();

/** Cleanup stratum server network connections and free resources. */
void StopStratumServer();

#endif // FREICOIN_STRATUM_H

// End of File
