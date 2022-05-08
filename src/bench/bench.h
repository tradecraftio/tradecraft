// Copyright (c) 2015-2020 The Bitcoin Core developers
// Copyright (c) 2011-2022 The Freicoin Developers
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

#ifndef BITCOIN_BENCH_BENCH_H
#define BITCOIN_BENCH_BENCH_H

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <bench/nanobench.h>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

/*
 * Usage:

static void CODE_TO_TIME(benchmark::Bench& bench)
{
    ... do any setup needed...
    nanobench::Config().run([&] {
       ... do stuff you want to time...
    });
    ... do any cleanup needed...
}

BENCHMARK(CODE_TO_TIME);

 */

namespace benchmark {

using ankerl::nanobench::Bench;

typedef std::function<void(Bench&)> BenchFunction;

struct Args {
    std::string regex_filter;
    bool is_list_only;
    std::vector<double> asymptote;
    std::string output_csv;
    std::string output_json;
};

class BenchRunner
{
    typedef std::map<std::string, BenchFunction> BenchmarkMap;
    static BenchmarkMap& benchmarks();

public:
    BenchRunner(std::string name, BenchFunction func);

    static void RunAll(const Args& args);
};
}
// BENCHMARK(foo) expands to:  benchmark::BenchRunner bench_11foo("foo");
#define BENCHMARK(n) \
    benchmark::BenchRunner BOOST_PP_CAT(bench_, BOOST_PP_CAT(__LINE__, n))(BOOST_PP_STRINGIZE(n), n);

#endif // BITCOIN_BENCH_BENCH_H
