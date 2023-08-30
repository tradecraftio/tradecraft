// Copyright (c) 2015-2022 The Bitcoin Core developers
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

#include <bench/bench.h>
#include <hash.h>
#include <random.h>
#include <uint256.h>


static void PrePadded(benchmark::Bench& bench)
{

    CSHA256 hasher;

    // Setup the salted hasher
    uint256 nonce = GetRandHash();
    hasher.Write(nonce.begin(), 32);
    hasher.Write(nonce.begin(), 32);
    uint256 data = GetRandHash();
    bench.run([&] {
        unsigned char out[32];
        CSHA256 h = hasher;
        h.Write(data.begin(), 32);
        h.Finalize(out);
    });
}

BENCHMARK(PrePadded, benchmark::PriorityLevel::HIGH);

static void RegularPadded(benchmark::Bench& bench)
{
    CSHA256 hasher;

    // Setup the salted hasher
    uint256 nonce = GetRandHash();
    uint256 data = GetRandHash();
    bench.run([&] {
        unsigned char out[32];
        CSHA256 h = hasher;
        h.Write(nonce.begin(), 32);
        h.Write(data.begin(), 32);
        h.Finalize(out);
    });
}

BENCHMARK(RegularPadded, benchmark::PriorityLevel::HIGH);
