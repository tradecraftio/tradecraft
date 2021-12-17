// Copyright (c) 2020 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <bloom.h>
#include <optional.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    CRollingBloomFilter rolling_bloom_filter{
        fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 1000),
        0.999 / fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, std::numeric_limits<unsigned int>::max())};
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange(0, 2)) {
        case 0: {
            const std::vector<unsigned char> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
            (void)rolling_bloom_filter.contains(b);
            rolling_bloom_filter.insert(b);
            const bool present = rolling_bloom_filter.contains(b);
            assert(present);
            break;
        }
        case 1: {
            const Optional<uint256> u256 = ConsumeDeserializable<uint256>(fuzzed_data_provider);
            if (!u256) {
                break;
            }
            (void)rolling_bloom_filter.contains(*u256);
            rolling_bloom_filter.insert(*u256);
            const bool present = rolling_bloom_filter.contains(*u256);
            assert(present);
            break;
        }
        case 2:
            rolling_bloom_filter.reset();
            break;
        }
    }
}
