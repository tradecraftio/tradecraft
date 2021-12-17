// Copyright (c) 2019 The Bitcoin Core developers
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
#include <crypto/chacha20.h>

/* Number of bytes to process per iteration */
static const uint64_t BUFFER_SIZE_TINY  = 64;
static const uint64_t BUFFER_SIZE_SMALL = 256;
static const uint64_t BUFFER_SIZE_LARGE = 1024*1024;

static void CHACHA20(benchmark::State& state, size_t buffersize)
{
    std::vector<uint8_t> key(32,0);
    ChaCha20 ctx(key.data(), key.size());
    ctx.SetIV(0);
    ctx.Seek(0);
    std::vector<uint8_t> in(buffersize,0);
    std::vector<uint8_t> out(buffersize,0);
    while (state.KeepRunning()) {
        ctx.Crypt(in.data(), out.data(), in.size());
    }
}

static void CHACHA20_64BYTES(benchmark::State& state)
{
    CHACHA20(state, BUFFER_SIZE_TINY);
}

static void CHACHA20_256BYTES(benchmark::State& state)
{
    CHACHA20(state, BUFFER_SIZE_SMALL);
}

static void CHACHA20_1MB(benchmark::State& state)
{
    CHACHA20(state, BUFFER_SIZE_LARGE);
}

BENCHMARK(CHACHA20_64BYTES, 500000);
BENCHMARK(CHACHA20_256BYTES, 250000);
BENCHMARK(CHACHA20_1MB, 340);
