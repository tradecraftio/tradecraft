// Copyright (c) 2016 The Bitcoin Core developers
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

#include "bench.h"

#include "chainparams.h"
#include "validation.h"
#include "streams.h"
#include "consensus/validation.h"

namespace block_bench {
#include "bench/data/block413567.raw.h"
}

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::State& state)
{
    CDataStream stream((const char*)block_bench::block413567,
            (const char*)&block_bench::block413567[sizeof(block_bench::block413567)],
            SER_NETWORK, PROTOCOL_VERSION);
    char a;
    stream.write(&a, 1); // Prevent compaction

    while (state.KeepRunning()) {
        CBlock block;
        stream >> block;
        assert(stream.Rewind(sizeof(block_bench::block413567)));
    }
}

static void DeserializeAndCheckBlockTest(benchmark::State& state)
{
    CDataStream stream((const char*)block_bench::block413567,
            (const char*)&block_bench::block413567[sizeof(block_bench::block413567)],
            SER_NETWORK, PROTOCOL_VERSION);
    char a;
    stream.write(&a, 1); // Prevent compaction

    Consensus::Params params = Params(CBaseChainParams::MAIN).GetConsensus();

    while (state.KeepRunning()) {
        CBlock block; // Note that CBlock caches its checked state, so we need to recreate it here
        stream >> block;
        assert(stream.Rewind(sizeof(block_bench::block413567)));

        CValidationState validationState;
        assert(CheckBlock(block, validationState, params));
    }
}

BENCHMARK(DeserializeBlockTest);
BENCHMARK(DeserializeAndCheckBlockTest);
