// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#include <net.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

FUZZ_TARGET(node_eviction)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<NodeEvictionCandidate> eviction_candidates;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        eviction_candidates.push_back({
            /*id=*/fuzzed_data_provider.ConsumeIntegral<NodeId>(),
            /*m_connected=*/std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<int64_t>()},
            /*m_min_ping_time=*/std::chrono::microseconds{fuzzed_data_provider.ConsumeIntegral<int64_t>()},
            /*m_last_block_time=*/std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<int64_t>()},
            /*m_last_tx_time=*/std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<int64_t>()},
            /*fRelevantServices=*/fuzzed_data_provider.ConsumeBool(),
            /*m_relay_txs=*/fuzzed_data_provider.ConsumeBool(),
            /*fBloomFilter=*/fuzzed_data_provider.ConsumeBool(),
            /*nKeyedNetGroup=*/fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
            /*prefer_evict=*/fuzzed_data_provider.ConsumeBool(),
            /*m_is_local=*/fuzzed_data_provider.ConsumeBool(),
            /*m_network=*/fuzzed_data_provider.PickValueInArray(ALL_NETWORKS),
            /*m_noban=*/fuzzed_data_provider.ConsumeBool(),
            /*m_conn_type=*/fuzzed_data_provider.PickValueInArray(ALL_CONNECTION_TYPES),
        });
    }
    // Make a copy since eviction_candidates may be in some valid but otherwise
    // indeterminate state after the SelectNodeToEvict(&&) call.
    const std::vector<NodeEvictionCandidate> eviction_candidates_copy = eviction_candidates;
    const std::optional<NodeId> node_to_evict = SelectNodeToEvict(std::move(eviction_candidates));
    if (node_to_evict) {
        assert(std::any_of(eviction_candidates_copy.begin(), eviction_candidates_copy.end(), [&node_to_evict](const NodeEvictionCandidate& eviction_candidate) { return *node_to_evict == eviction_candidate.id; }));
    }
}
