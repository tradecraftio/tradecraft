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

#include <util/time.h>

static void BenchTimeDeprecated(benchmark::State& state)
{
    while (state.KeepRunning()) {
        (void)GetTime();
    }
}

static void BenchTimeMock(benchmark::State& state)
{
    SetMockTime(111);
    while (state.KeepRunning()) {
        (void)GetTime<std::chrono::seconds>();
    }
    SetMockTime(0);
}

static void BenchTimeMillis(benchmark::State& state)
{
    while (state.KeepRunning()) {
        (void)GetTime<std::chrono::milliseconds>();
    }
}

static void BenchTimeMillisSys(benchmark::State& state)
{
    while (state.KeepRunning()) {
        (void)GetTimeMillis();
    }
}

BENCHMARK(BenchTimeDeprecated, 100000000);
BENCHMARK(BenchTimeMillis, 6000000);
BENCHMARK(BenchTimeMillisSys, 6000000);
BENCHMARK(BenchTimeMock, 300000000);
