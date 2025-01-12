// Copyright (c) 2024-present The Bitcoin Core developers
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

#include <node/timeoffsets.h>
#include <node/warnings.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>

#include <chrono>
#include <cstdint>
#include <functional>

void initialize_timeoffsets()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::MAIN);
}

FUZZ_TARGET(timeoffsets, .init = initialize_timeoffsets)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    node::Warnings warnings{};
    TimeOffsets offsets{warnings};
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 4'000) {
        (void)offsets.Median();
        offsets.Add(std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<std::chrono::seconds::rep>()});
        offsets.WarnIfOutOfSync();
    }
}
