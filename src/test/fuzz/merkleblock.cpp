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

#include <merkleblock.h>
#include <optional.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    Optional<CPartialMerkleTree> partial_merkle_tree = ConsumeDeserializable<CPartialMerkleTree>(fuzzed_data_provider);
    if (!partial_merkle_tree) {
        return;
    }
    (void)partial_merkle_tree->GetNumTransactions();
    std::vector<uint256> matches;
    std::vector<unsigned int> indices;
    (void)partial_merkle_tree->ExtractMatches(matches, indices);
}
