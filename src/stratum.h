// Copyright (c) 2020-2022 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BITCOIN_STRATUM_H
#define BITCOIN_STRATUM_H

#include <node/context.h>
#include <uint256.h>

#include <univalue.h>

#include <stdint.h>

#include <string>

/** JSON ser/deser utility functions. */
std::string HexInt4(uint32_t val);
uint32_t ParseHexInt4(const UniValue& hex, const std::string& name);
uint256 ParseUInt256(const UniValue& hex, const std::string& name);

/** Check that the number of parameters are within [min, max]. */
void BoundParams(const std::string& method, const UniValue& params, size_t min, size_t max);

/** Configure the stratum server. */
bool InitStratumServer(node::NodeContext& node);

/** Interrupt the stratum server connections. */
void InterruptStratumServer();

/** Cleanup stratum server network connections and free resources. */
void StopStratumServer();

#endif // BITCOIN_STRATUM_H

// End of File
