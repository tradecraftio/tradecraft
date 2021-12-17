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

#include <optional.h>
#include <primitives/block.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const Optional<CBlockHeader> block_header = ConsumeDeserializable<CBlockHeader>(fuzzed_data_provider);
    if (!block_header) {
        return;
    }
    {
        const uint256 hash = block_header->GetHash();
        static const uint256 u256_max(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
        assert(hash != u256_max);
        assert(block_header->GetBlockTime() == block_header->nTime);
        assert(block_header->IsNull() == (block_header->nBits == 0));
    }
    {
        CBlockHeader mut_block_header = *block_header;
        mut_block_header.SetNull();
        assert(mut_block_header.IsNull());
        CBlock block{*block_header};
        assert(block.GetBlockHeader().GetHash() == block_header->GetHash());
        (void)block.ToString();
        block.SetNull();
        assert(block.GetBlockHeader().GetHash() == mut_block_header.GetHash());
    }
}
