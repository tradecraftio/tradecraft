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

#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

FUZZ_TARGET(buffered_file)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider = ConsumeFile(fuzzed_data_provider);
    std::optional<CBufferedFile> opt_buffered_file;
    FILE* fuzzed_file = fuzzed_file_provider.open();
    try {
        opt_buffered_file.emplace(fuzzed_file, fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096), fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096), fuzzed_data_provider.ConsumeIntegral<int>(), fuzzed_data_provider.ConsumeIntegral<int>());
    } catch (const std::ios_base::failure&) {
        if (fuzzed_file != nullptr) {
            fclose(fuzzed_file);
        }
    }
    if (opt_buffered_file && fuzzed_file != nullptr) {
        bool setpos_fail = false;
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    std::array<std::byte, 4096> arr{};
                    try {
                        opt_buffered_file->read({arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)});
                    } catch (const std::ios_base::failure&) {
                    }
                },
                [&] {
                    opt_buffered_file->SetLimit(fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096));
                },
                [&] {
                    if (!opt_buffered_file->SetPos(fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096))) {
                        setpos_fail = true;
                    }
                },
                [&] {
                    if (setpos_fail) {
                        // Calling FindByte(...) after a failed SetPos(...) call may result in an infinite loop.
                        return;
                    }
                    try {
                        opt_buffered_file->FindByte(fuzzed_data_provider.ConsumeIntegral<uint8_t>());
                    } catch (const std::ios_base::failure&) {
                    }
                },
                [&] {
                    ReadFromStream(fuzzed_data_provider, *opt_buffered_file);
                });
        }
        opt_buffered_file->GetPos();
        opt_buffered_file->GetType();
        opt_buffered_file->GetVersion();
    }
}
