// Copyright (c) 2016-2022 The Bitcoin Core developers
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

#include <support/lockedpool.h>

#include <vector>

#define ASIZE 2048
#define MSIZE 2048

static void BenchLockedPool(benchmark::Bench& bench)
{
    void *synth_base = reinterpret_cast<void*>(0x08000000);
    const size_t synth_size = 1024*1024;
    Arena b(synth_base, synth_size, 16);

    std::vector<void*> addr{ASIZE, nullptr};
    uint32_t s = 0x12345678;
    bench.run([&] {
        int idx = s & (addr.size() - 1);
        if (s & 0x80000000) {
            b.free(addr[idx]);
            addr[idx] = nullptr;
        } else if (!addr[idx]) {
            addr[idx] = b.alloc((s >> 16) & (MSIZE - 1));
        }
        bool lsb = s & 1;
        s >>= 1;
        if (lsb)
            s ^= 0xf00f00f0; // LFSR period 0xf7ffffe0
    });
    for (void *ptr: addr)
        b.free(ptr);
    addr.clear();
}

BENCHMARK(BenchLockedPool, benchmark::PriorityLevel::HIGH);
