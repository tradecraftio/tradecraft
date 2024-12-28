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

#include <span.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <vector>

FUZZ_TARGET(autofile)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    AutoFile auto_file{
        fuzzed_file_provider.open(),
        ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider),
    };
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                std::array<std::byte, 4096> arr{};
                try {
                    auto_file.read({arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)});
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                const std::array<std::byte, 4096> arr{};
                try {
                    auto_file.write({arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)});
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                try {
                    auto_file.ignore(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                auto_file.fclose();
            },
            [&] {
                ReadFromStream(fuzzed_data_provider, auto_file);
            },
            [&] {
                WriteToStream(fuzzed_data_provider, auto_file);
            });
    }
    (void)auto_file.Get();
    (void)auto_file.IsNull();
    if (fuzzed_data_provider.ConsumeBool()) {
        FILE* f = auto_file.release();
        if (f != nullptr) {
            fclose(f);
        }
    }
}
