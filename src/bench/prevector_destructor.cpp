// Copyright (c) 2015-2017 The Bitcoin Core developers
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
#include <prevector.h>

static void PrevectorDestructor(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (auto x = 0; x < 1000; ++x) {
            prevector<28, unsigned char> t0;
            prevector<28, unsigned char> t1;
            t0.resize(28);
            t1.resize(29);
        }
    }
}

static void PrevectorClear(benchmark::State& state)
{

    while (state.KeepRunning()) {
        for (auto x = 0; x < 1000; ++x) {
            prevector<28, unsigned char> t0;
            prevector<28, unsigned char> t1;
            t0.resize(28);
            t0.clear();
            t1.resize(29);
            t0.clear();
        }
    }
}

BENCHMARK(PrevectorDestructor, 5700);
BENCHMARK(PrevectorClear, 5600);
