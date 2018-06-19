// Copyright (c) 2016-2019 The Bitcoin Core developers
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

#include <bench/bench.h>
#include <bench/data.h>

#include <validation.h>
#include <streams.h>
#include <consensus/validation.h>
#include <rpc/blockchain.h>

#include <univalue.h>

static void BlockToJsonVerbose(benchmark::State& state) {
    CDataStream stream(benchmark::data::block136207, SER_NETWORK, PROTOCOL_VERSION);
    char a = '\0';
    stream.write(&a, 1); // Prevent compaction

    CBlock block;
    stream >> block;

    CBlockIndex blockindex;
    const uint256 blockHash = block.GetHash();
    blockindex.phashBlock = &blockHash;
    blockindex.nBits = 403014710;

    while (state.KeepRunning()) {
        (void)blockToJSON(block, &blockindex, &blockindex, /*verbose*/ true);
    }
}

BENCHMARK(BlockToJsonVerbose, 10);
