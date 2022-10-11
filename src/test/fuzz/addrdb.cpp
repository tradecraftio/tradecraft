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

#include <addrdb.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // The point of this code is to exercise all CBanEntry constructors.
    const CBanEntry ban_entry = [&] {
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 2)) {
        case 0:
            return CBanEntry{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
            break;
        case 1: {
            const std::optional<CBanEntry> ban_entry = ConsumeDeserializable<CBanEntry>(fuzzed_data_provider);
            if (ban_entry) {
                return *ban_entry;
            }
            break;
        }
        }
        return CBanEntry{};
    }();
    (void)ban_entry; // currently unused
}
