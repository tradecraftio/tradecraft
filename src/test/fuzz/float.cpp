// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <memusage.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/serfloat.h>

#include <cassert>
#include <cmath>
#include <limits>

FUZZ_TARGET(float)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    {
        const double d{[&] {
            double tmp;
            CallOneOf(
                fuzzed_data_provider,
                // an actual number
                [&] { tmp = fuzzed_data_provider.ConsumeFloatingPoint<double>(); },
                // special numbers and NANs
                [&] { tmp = fuzzed_data_provider.PickValueInArray({
                          std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::min(),
                          -std::numeric_limits<double>::min(),
                          std::numeric_limits<double>::max(),
                          -std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::lowest(),
                          -std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::quiet_NaN(),
                          -std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::signaling_NaN(),
                          -std::numeric_limits<double>::signaling_NaN(),
                          std::numeric_limits<double>::denorm_min(),
                          -std::numeric_limits<double>::denorm_min(),
                      }); },
                // Anything from raw memory (also checks that DecodeDouble doesn't crash on any input)
                [&] { tmp = DecodeDouble(fuzzed_data_provider.ConsumeIntegral<uint64_t>()); });
            return tmp;
        }()};
        (void)memusage::DynamicUsage(d);

        uint64_t encoded = EncodeDouble(d);
        if constexpr (std::numeric_limits<double>::is_iec559) {
            if (!std::isnan(d)) {
                uint64_t encoded_in_memory;
                std::copy((const unsigned char*)&d, (const unsigned char*)(&d + 1), (unsigned char*)&encoded_in_memory);
                assert(encoded_in_memory == encoded);
            }
        }
        double d_deserialized = DecodeDouble(encoded);
        assert(std::isnan(d) == std::isnan(d_deserialized));
        assert(std::isnan(d) || d == d_deserialized);
    }
}
