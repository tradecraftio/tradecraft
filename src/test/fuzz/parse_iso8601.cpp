// Copyright (c) 2019-2021 The Bitcoin Core developers
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

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/time.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(parse_iso8601)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const int64_t random_time = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    const std::string random_string = fuzzed_data_provider.ConsumeRemainingBytesAsString();

    const std::string iso8601_datetime = FormatISO8601DateTime(random_time);
    (void)FormatISO8601Date(random_time);
    const int64_t parsed_time_1 = ParseISO8601DateTime(iso8601_datetime);
    if (random_time >= 0) {
        assert(parsed_time_1 >= 0);
        if (iso8601_datetime.length() == 20) {
            assert(parsed_time_1 == random_time);
        }
    }

    const int64_t parsed_time_2 = ParseISO8601DateTime(random_string);
    assert(parsed_time_2 >= 0);
}
