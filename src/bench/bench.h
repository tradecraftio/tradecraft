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

#ifndef FREICOIN_BENCH_BENCH_H
#define FREICOIN_BENCH_BENCH_H

#include <util/fs.h>
#include <util/macros.h>

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <bench/nanobench.h>

/*
 * Usage:

static void NameOfYourBenchmarkFunction(benchmark::Bench& bench)
{
    ...do any setup needed...

    bench.run([&] {
         ...do stuff you want to time; refer to src/bench/nanobench.h
            for more information and the options that can be passed here...
    });

    ...do any cleanup needed...
}

BENCHMARK(NameOfYourBenchmarkFunction);

 */

namespace benchmark {

using ankerl::nanobench::Bench;

typedef std::function<void(Bench&)> BenchFunction;

enum PriorityLevel : uint8_t
{
    LOW = 1 << 0,
    HIGH = 1 << 2,
};

// List priority labels, comma-separated and sorted by increasing priority
std::string ListPriorities();
uint8_t StringToPriority(const std::string& str);

struct Args {
    bool is_list_only;
    bool sanity_check;
    std::chrono::milliseconds min_time;
    std::vector<double> asymptote;
    fs::path output_csv;
    fs::path output_json;
    std::string regex_filter;
    uint8_t priority;
};

class BenchRunner
{
    // maps from "name" -> (function, priority_level)
    typedef std::map<std::string, std::pair<BenchFunction, PriorityLevel>> BenchmarkMap;
    static BenchmarkMap& benchmarks();

public:
    BenchRunner(std::string name, BenchFunction func, PriorityLevel level);

    static void RunAll(const Args& args);
};
} // namespace benchmark

// BENCHMARK(foo) expands to:  benchmark::BenchRunner bench_11foo("foo", foo, priority_level);
#define BENCHMARK(n, priority_level) \
    benchmark::BenchRunner PASTE2(bench_, PASTE2(__LINE__, n))(STRINGIZE(n), n, priority_level);

#endif // FREICOIN_BENCH_BENCH_H
