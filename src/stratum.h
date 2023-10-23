// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
