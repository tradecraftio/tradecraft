// Copyright (c) 2016 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include <iostream>

#include "bench.h"
#include "bloom.h"
#include "hash.h"
#include "uint256.h"
#include "utiltime.h"
#include "crypto/ripemd160.h"
#include "crypto/sha1.h"
#include "crypto/sha256.h"
#include "crypto/sha512.h"

/* Number of bytes to hash per iteration */
static const uint64_t BUFFER_SIZE = 1000*1000;

static void RIPEMD160(benchmark::State& state)
{
    uint8_t hash[CRIPEMD160::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CRIPEMD160().Write(begin_ptr(in), in.size()).Finalize(hash);
}

static void SHA1(benchmark::State& state)
{
    uint8_t hash[CSHA1::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA1().Write(begin_ptr(in), in.size()).Finalize(hash);
}

static void SHA256(benchmark::State& state)
{
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA256().Write(begin_ptr(in), in.size()).Finalize(hash);
}

static void SHA256_32b(benchmark::State& state)
{
    std::vector<uint8_t> in(32,0);
    while (state.KeepRunning()) {
        for (int i = 0; i < 1000000; i++) {
            CSHA256().Write(begin_ptr(in), in.size()).Finalize(&in[0]);
        }
    }
}

static void SHA512(benchmark::State& state)
{
    uint8_t hash[CSHA512::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA512().Write(begin_ptr(in), in.size()).Finalize(hash);
}

static void SipHash_32b(benchmark::State& state)
{
    uint256 x;
    while (state.KeepRunning()) {
        for (int i = 0; i < 1000000; i++) {
            *((uint64_t*)x.begin()) = SipHashUint256(0, i, x);
        }
    }
}

BENCHMARK(RIPEMD160);
BENCHMARK(SHA1);
BENCHMARK(SHA256);
BENCHMARK(SHA512);

BENCHMARK(SHA256_32b);
BENCHMARK(SipHash_32b);
