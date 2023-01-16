// Copyright (c) 2022 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
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
#ifndef FREICOIN_KERNEL_MEMPOOL_OPTIONS_H
#define FREICOIN_KERNEL_MEMPOOL_OPTIONS_H

#include <kernel/mempool_limits.h>

#include <policy/feerate.h>
#include <policy/policy.h>
#include <script/standard.h>

#include <chrono>
#include <cstdint>
#include <optional>

class CBlockPolicyEstimator;

/** Default for -maxmempool, maximum megabytes of mempool memory usage */
static constexpr unsigned int DEFAULT_MAX_MEMPOOL_SIZE_MB{300};
/** Default for -maxmempool when blocksonly is set */
static constexpr unsigned int DEFAULT_BLOCKSONLY_MAX_MEMPOOL_SIZE_MB{5};
/** Default for -mempoolexpiry, expiration time for mempool transactions in hours */
static constexpr unsigned int DEFAULT_MEMPOOL_EXPIRY_HOURS{336};
/** Default for -mempoolfullrbf, if the transaction replaceability signaling is ignored */
static constexpr bool DEFAULT_MEMPOOL_FULL_RBF{false};

namespace kernel {
/**
 * Options struct containing options for constructing a CTxMemPool. Default
 * constructor populates the struct with sane default values which can be
 * modified.
 *
 * Most of the time, this struct should be referenced as CTxMemPool::Options.
 */
struct MemPoolOptions {
    /* Used to estimate appropriate transaction fees. */
    CBlockPolicyEstimator* estimator{nullptr};
    /* The ratio used to determine how often sanity checks will run.  */
    int check_ratio{0};
    int64_t max_size_bytes{DEFAULT_MAX_MEMPOOL_SIZE_MB * 1'000'000};
    std::chrono::seconds expiry{std::chrono::hours{DEFAULT_MEMPOOL_EXPIRY_HOURS}};
    CFeeRate incremental_relay_feerate{DEFAULT_INCREMENTAL_RELAY_FEE};
    /** A fee rate smaller than this is considered zero fee (for relaying, mining and transaction creation) */
    CFeeRate min_relay_feerate{DEFAULT_MIN_RELAY_TX_FEE};
    CFeeRate dust_relay_feerate{DUST_RELAY_TX_FEE};
    /**
     * A data carrying output is an unspendable output containing data. The script
     * type is designated as TxoutType::NULL_DATA.
     *
     * Maximum size of TxoutType::NULL_DATA scripts that this node considers standard.
     * If nullopt, any size (other than zero) is nonstandard.
     *
     * Zero-sized OP_RETURN outputs are classed as TxoutType::UNSPENDABLE
     * and are always allowed as a way of destroying coin.
     */
    std::optional<unsigned> max_datacarrier_bytes{DEFAULT_ACCEPT_DATACARRIER ? std::optional{MAX_OP_RETURN_RELAY} : std::nullopt};
    bool permit_bare_multisig{DEFAULT_PERMIT_BAREMULTISIG};
    bool require_standard{true};
    bool full_rbf{DEFAULT_MEMPOOL_FULL_RBF};
    MemPoolLimits limits{};
};
} // namespace kernel

#endif // FREICOIN_KERNEL_MEMPOOL_OPTIONS_H
