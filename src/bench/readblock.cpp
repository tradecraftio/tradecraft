// Copyright (c) 2023 The Bitcoin Core developers
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

#include <bench/bench.h>
#include <bench/data.h>

#include <consensus/validation.h>
#include <node/blockstorage.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>
#include <validation.h>

static FlatFilePos WriteBlockToDisk(ChainstateManager& chainman)
{
    DataStream stream{benchmark::data::block413567};
    CBlock block;
    stream >> TX_WITH_WITNESS(block);

    return chainman.m_blockman.SaveBlockToDisk(block, 0);
}

static void ReadBlockFromDiskTest(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    ChainstateManager& chainman{*testing_setup->m_node.chainman};

    CBlock block;
    const auto pos{WriteBlockToDisk(chainman)};

    bench.run([&] {
        const auto success{chainman.m_blockman.ReadBlockFromDisk(block, pos)};
        assert(success);
    });
}

static void ReadRawBlockFromDiskTest(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    ChainstateManager& chainman{*testing_setup->m_node.chainman};

    std::vector<uint8_t> block_data;
    const auto pos{WriteBlockToDisk(chainman)};

    bench.run([&] {
        const auto success{chainman.m_blockman.ReadRawBlockFromDisk(block_data, pos)};
        assert(success);
    });
}

BENCHMARK(ReadBlockFromDiskTest, benchmark::PriorityLevel::HIGH);
BENCHMARK(ReadRawBlockFromDiskTest, benchmark::PriorityLevel::HIGH);
