// Copyright (c) 2021 The Bitcoin Core developers
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

#ifndef BITCOIN_NODE_CHAINSTATE_H
#define BITCOIN_NODE_CHAINSTATE_H

#include <cstdint>
#include <functional>
#include <optional>

class ChainstateManager;
class CTxMemPool;
namespace Consensus {
struct Params;
} // namespace Consensus

namespace node {
enum class ChainstateLoadingError {
    ERROR_LOADING_BLOCK_DB,
    ERROR_BAD_GENESIS_BLOCK,
    ERROR_PRUNED_NEEDS_REINDEX,
    ERROR_CHAINSTATE_NEEDS_REINDEX,
    ERROR_LOAD_GENESIS_BLOCK_FAILED,
    ERROR_CHAINSTATE_UPGRADE_FAILED,
    ERROR_REPLAYBLOCKS_FAILED,
    ERROR_LOADCHAINTIP_FAILED,
    ERROR_GENERIC_BLOCKDB_OPEN_FAILED,
    ERROR_BLOCKS_WITNESS_INSUFFICIENTLY_VALIDATED,
    SHUTDOWN_PROBED,
};

/** This sequence can have 4 types of outcomes:
 *
 *  1. Success
 *  2. Shutdown requested
 *    - nothing failed but a shutdown was triggered in the middle of the
 *      sequence
 *  3. Soft failure
 *    - a failure that might be recovered from with a reindex
 *  4. Hard failure
 *    - a failure that definitively cannot be recovered from with a reindex
 *
 *  Currently, LoadChainstate returns a std::optional<ChainstateLoadingError>
 *  which:
 *
 *  - if has_value()
 *      - Either "Soft failure", "Hard failure", or "Shutdown requested",
 *        differentiable by the specific enumerator.
 *
 *        Note that a return value of SHUTDOWN_PROBED means ONLY that "during
 *        this sequence, when we explicitly checked shutdown_requested() at
 *        arbitrary points, one of those calls returned true". Therefore, a
 *        return value other than SHUTDOWN_PROBED does not guarantee that
 *        shutdown hasn't been called indirectly.
 *  - else
 *      - Success!
 */
std::optional<ChainstateLoadingError> LoadChainstate(bool fReset,
                                                     ChainstateManager& chainman,
                                                     CTxMemPool* mempool,
                                                     bool fPruneMode,
                                                     const Consensus::Params& consensus_params,
                                                     bool fReindexChainState,
                                                     int64_t nBlockTreeDBCache,
                                                     int64_t nCoinDBCache,
                                                     int64_t nCoinCacheUsage,
                                                     bool block_tree_db_in_memory,
                                                     bool coins_db_in_memory,
                                                     std::function<bool()> shutdown_requested = nullptr,
                                                     std::function<void()> coins_error_cb = nullptr);

enum class ChainstateLoadVerifyError {
    ERROR_BLOCK_FROM_FUTURE,
    ERROR_CORRUPTED_BLOCK_DB,
    ERROR_GENERIC_FAILURE,
};

std::optional<ChainstateLoadVerifyError> VerifyLoadedChainstate(ChainstateManager& chainman,
                                                                bool fReset,
                                                                bool fReindexChainState,
                                                                const Consensus::Params& consensus_params,
                                                                int check_blocks,
                                                                int check_level,
                                                                std::function<int64_t()> get_unix_time_seconds);
} // namespace node

#endif // BITCOIN_NODE_CHAINSTATE_H
