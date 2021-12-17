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
#include <hash.h>
#include <random.h>
#include <uint256.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <crypto/sha512.h>
#include <crypto/siphash.h>

/* Number of bytes to hash per iteration */
static const uint64_t BUFFER_SIZE = 1000*1000;

static void RIPEMD160(benchmark::State& state)
{
    uint8_t hash[CRIPEMD160::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CRIPEMD160().Write(in.data(), in.size()).Finalize(hash);
}

static void SHA1(benchmark::State& state)
{
    uint8_t hash[CSHA1::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA1().Write(in.data(), in.size()).Finalize(hash);
}

static void SHA256(benchmark::State& state)
{
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA256().Write(in.data(), in.size()).Finalize(hash);
}

static void SHA256_32b(benchmark::State& state)
{
    std::vector<uint8_t> in(32,0);
    while (state.KeepRunning()) {
        CSHA256()
            .Write(in.data(), in.size())
            .Finalize(in.data());
    }
}

static void SHA256D64_1024(benchmark::State& state)
{
    std::vector<uint8_t> in(64 * 1024, 0);
    while (state.KeepRunning()) {
        SHA256D64(in.data(), in.data(), 1024);
    }
}

static void SHA512(benchmark::State& state)
{
    uint8_t hash[CSHA512::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA512().Write(in.data(), in.size()).Finalize(hash);
}

static void SipHash_32b(benchmark::State& state)
{
    uint256 x;
    uint64_t k1 = 0;
    while (state.KeepRunning()) {
        *((uint64_t*)x.begin()) = SipHashUint256(0, ++k1, x);
    }
}

static void FastRandom_32bit(benchmark::State& state)
{
    FastRandomContext rng(true);
    while (state.KeepRunning()) {
        rng.rand32();
    }
}

static void FastRandom_1bit(benchmark::State& state)
{
    FastRandomContext rng(true);
    while (state.KeepRunning()) {
        rng.randbool();
    }
}

BENCHMARK(RIPEMD160, 440);
BENCHMARK(SHA1, 570);
BENCHMARK(SHA256, 340);
BENCHMARK(SHA512, 330);

BENCHMARK(SHA256_32b, 4700 * 1000);
BENCHMARK(SipHash_32b, 40 * 1000 * 1000);
BENCHMARK(SHA256D64_1024, 7400);
BENCHMARK(FastRandom_32bit, 110 * 1000 * 1000);
BENCHMARK(FastRandom_1bit, 440 * 1000 * 1000);
