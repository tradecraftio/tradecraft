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

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <timedata.h>

#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const unsigned int max_size = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 1000);
    // Divide by 2 to avoid signed integer overflow in .median()
    const int64_t initial_value = fuzzed_data_provider.ConsumeIntegral<int64_t>() / 2;
    CMedianFilter<int64_t> median_filter{max_size, initial_value};
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        (void)median_filter.median();
        assert(median_filter.size() > 0);
        assert(static_cast<size_t>(median_filter.size()) == median_filter.sorted().size());
        assert(static_cast<unsigned int>(median_filter.size()) <= max_size || max_size == 0);
        // Divide by 2 to avoid signed integer overflow in .median()
        median_filter.input(fuzzed_data_provider.ConsumeIntegral<int64_t>() / 2);
    }
}
