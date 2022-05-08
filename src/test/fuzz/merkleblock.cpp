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

#include <merkleblock.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CPartialMerkleTree partial_merkle_tree;
    switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 1)) {
    case 0: {
        const std::optional<CPartialMerkleTree> opt_partial_merkle_tree = ConsumeDeserializable<CPartialMerkleTree>(fuzzed_data_provider);
        if (opt_partial_merkle_tree) {
            partial_merkle_tree = *opt_partial_merkle_tree;
        }
        break;
    }
    case 1: {
        CMerkleBlock merkle_block;
        const std::optional<CBlock> opt_block = ConsumeDeserializable<CBlock>(fuzzed_data_provider);
        CBloomFilter bloom_filter;
        std::set<uint256> txids;
        if (opt_block && !opt_block->vtx.empty()) {
            if (fuzzed_data_provider.ConsumeBool()) {
                merkle_block = CMerkleBlock{*opt_block, bloom_filter};
            } else if (fuzzed_data_provider.ConsumeBool()) {
                while (fuzzed_data_provider.ConsumeBool()) {
                    txids.insert(ConsumeUInt256(fuzzed_data_provider));
                }
                merkle_block = CMerkleBlock{*opt_block, txids};
            }
        }
        partial_merkle_tree = merkle_block.txn;
        break;
    }
    }
    (void)partial_merkle_tree.GetNumTransactions();
    std::vector<uint256> matches;
    std::vector<unsigned int> indices;
    (void)partial_merkle_tree.ExtractMatches(matches, indices);
}
