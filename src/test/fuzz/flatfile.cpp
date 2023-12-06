// Copyright (c) 2020 The Bitcoin Core developers
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

#include <flatfile.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

FUZZ_TARGET(flatfile)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::optional<FlatFilePos> flat_file_pos = ConsumeDeserializable<FlatFilePos>(fuzzed_data_provider);
    if (!flat_file_pos) {
        return;
    }
    std::optional<FlatFilePos> another_flat_file_pos = ConsumeDeserializable<FlatFilePos>(fuzzed_data_provider);
    if (another_flat_file_pos) {
        assert((*flat_file_pos == *another_flat_file_pos) != (*flat_file_pos != *another_flat_file_pos));
    }
    (void)flat_file_pos->ToString();
}
