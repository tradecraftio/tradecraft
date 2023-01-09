// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <cuckoocache.h>
#include <script/sigcache.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
FuzzedDataProvider* fuzzed_data_provider_ptr = nullptr;

struct RandomHasher {
    template <uint8_t>
    uint32_t operator()(const bool& /* unused */) const
    {
        assert(fuzzed_data_provider_ptr != nullptr);
        return fuzzed_data_provider_ptr->ConsumeIntegral<uint32_t>();
    }
};
} // namespace

FUZZ_TARGET(cuckoocache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    fuzzed_data_provider_ptr = &fuzzed_data_provider;
    CuckooCache::cache<int, RandomHasher> cuckoo_cache{};
    if (fuzzed_data_provider.ConsumeBool()) {
        const size_t megabytes = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 16);
        cuckoo_cache.setup_bytes(megabytes << 20);
    } else {
        cuckoo_cache.setup(fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 4096));
    }
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        if (fuzzed_data_provider.ConsumeBool()) {
            cuckoo_cache.insert(fuzzed_data_provider.ConsumeBool());
        } else {
            cuckoo_cache.contains(fuzzed_data_provider.ConsumeBool(), fuzzed_data_provider.ConsumeBool());
        }
    }
    fuzzed_data_provider_ptr = nullptr;
}
