// Copyright (c) 2020 The Bitcoin Core developers
// Copyright (c) 2011-2022 The Freicoin Developers
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
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    CBloomFilter bloom_filter{
        fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 10000000),
        1.0 / fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, std::numeric_limits<unsigned int>::max()),
        fuzzed_data_provider.ConsumeIntegral<unsigned int>(),
        static_cast<unsigned char>(fuzzed_data_provider.PickValueInArray({BLOOM_UPDATE_NONE, BLOOM_UPDATE_ALL, BLOOM_UPDATE_P2PUBKEY_ONLY, BLOOM_UPDATE_MASK}))};
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange(0, 3)) {
        case 0: {
            const std::vector<unsigned char> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
            (void)bloom_filter.contains(b);
            bloom_filter.insert(b);
            const bool present = bloom_filter.contains(b);
            assert(present);
            break;
        }
        case 1: {
            const std::optional<COutPoint> out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
            if (!out_point) {
                break;
            }
            (void)bloom_filter.contains(*out_point);
            bloom_filter.insert(*out_point);
            const bool present = bloom_filter.contains(*out_point);
            assert(present);
            break;
        }
        case 2: {
            const std::optional<uint256> u256 = ConsumeDeserializable<uint256>(fuzzed_data_provider);
            if (!u256) {
                break;
            }
            (void)bloom_filter.contains(*u256);
            bloom_filter.insert(*u256);
            const bool present = bloom_filter.contains(*u256);
            assert(present);
            break;
        }
        case 3: {
            const std::optional<CMutableTransaction> mut_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
            if (!mut_tx) {
                break;
            }
            const CTransaction tx{*mut_tx};
            (void)bloom_filter.IsRelevantAndUpdate(tx);
            break;
        }
        }
        (void)bloom_filter.IsWithinSizeConstraints();
    }
}
