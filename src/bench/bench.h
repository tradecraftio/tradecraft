// Copyright (c) 2015-2016 The Bitcoin Core developers
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

#ifndef FREICOIN_BENCH_BENCH_H
#define FREICOIN_BENCH_BENCH_H

#include <map>
#include <string>

#include <boost/function.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

// Simple micro-benchmarking framework; API mostly matches a subset of the Google Benchmark
// framework (see https://github.com/google/benchmark)
// Wny not use the Google Benchmark framework? Because adding Yet Another Dependency
// (that uses cmake as its build system and has lots of features we don't need) isn't
// worth it.

/*
 * Usage:

static void CODE_TO_TIME(benchmark::State& state)
{
    ... do any setup needed...
    while (state.KeepRunning()) {
       ... do stuff you want to time...
    }
    ... do any cleanup needed...
}

BENCHMARK(CODE_TO_TIME);

 */
 
namespace benchmark {

    class State {
        std::string name;
        double maxElapsed;
        double beginTime;
        double lastTime, minTime, maxTime, countMaskInv;
        uint64_t count;
        uint64_t countMask;
        uint64_t beginCycles;
        uint64_t lastCycles;
        uint64_t minCycles;
        uint64_t maxCycles;
    public:
        State(std::string _name, double _maxElapsed) : name(_name), maxElapsed(_maxElapsed), count(0) {
            minTime = std::numeric_limits<double>::max();
            maxTime = std::numeric_limits<double>::min();
            minCycles = std::numeric_limits<uint64_t>::max();
            maxCycles = std::numeric_limits<uint64_t>::min();
            countMask = 1;
            countMaskInv = 1./(countMask + 1);
        }
        bool KeepRunning();
    };

    typedef boost::function<void(State&)> BenchFunction;

    class BenchRunner
    {
        typedef std::map<std::string, BenchFunction> BenchmarkMap;
        static BenchmarkMap &benchmarks();

    public:
        BenchRunner(std::string name, BenchFunction func);

        static void RunAll(double elapsedTimeForOne=1.0);
    };
}

// BENCHMARK(foo) expands to:  benchmark::BenchRunner bench_11foo("foo", foo);
#define BENCHMARK(n) \
    benchmark::BenchRunner BOOST_PP_CAT(bench_, BOOST_PP_CAT(__LINE__, n))(BOOST_PP_STRINGIZE(n), n);

#endif // FREICOIN_BENCH_BENCH_H
