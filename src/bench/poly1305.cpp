// Copyright (c) 2019-2022 The Bitcoin Core developers
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
#include <crypto/poly1305.h>

#include <span.h>

/* Number of bytes to process per iteration */
static constexpr uint64_t BUFFER_SIZE_TINY  = 64;
static constexpr uint64_t BUFFER_SIZE_SMALL = 256;
static constexpr uint64_t BUFFER_SIZE_LARGE = 1024*1024;

static void POLY1305(benchmark::Bench& bench, size_t buffersize)
{
    std::vector<std::byte> tag(Poly1305::TAGLEN, {});
    std::vector<std::byte> key(Poly1305::KEYLEN, {});
    std::vector<std::byte> in(buffersize, {});
    bench.batch(in.size()).unit("byte").run([&] {
        Poly1305{key}.Update(in).Finalize(tag);
    });
}

static void POLY1305_64BYTES(benchmark::Bench& bench)
{
    POLY1305(bench, BUFFER_SIZE_TINY);
}

static void POLY1305_256BYTES(benchmark::Bench& bench)
{
    POLY1305(bench, BUFFER_SIZE_SMALL);
}

static void POLY1305_1MB(benchmark::Bench& bench)
{
    POLY1305(bench, BUFFER_SIZE_LARGE);
}

BENCHMARK(POLY1305_64BYTES, benchmark::PriorityLevel::HIGH);
BENCHMARK(POLY1305_256BYTES, benchmark::PriorityLevel::HIGH);
BENCHMARK(POLY1305_1MB, benchmark::PriorityLevel::HIGH);
