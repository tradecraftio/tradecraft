// Copyright (c) 2020 The Bitcoin Core developers
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
#include <logging.h>
#include <test/util/setup_common.h>


static void Logging(benchmark::Bench& bench, const std::vector<const char*>& extra_args, const std::function<void()>& log)
{
    TestingSetup test_setup{
        CBaseChainParams::REGTEST,
        extra_args,
    };

    bench.run([&] { log(); });
}

static void LoggingYoThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1"}, [] { LogPrintf("%s\n", "test"); });
}
static void LoggingNoThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0"}, [] { LogPrintf("%s\n", "test"); });
}
static void LoggingYoCategory(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=net"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}
static void LoggingNoCategory(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=0"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}
static void LoggingNoFile(benchmark::Bench& bench)
{
    Logging(bench, {"-nodebuglogfile", "-debug=1"}, [] {
        LogPrintf("%s\n", "test");
        LogPrint(BCLog::NET, "%s\n", "test");
    });
}

BENCHMARK(LoggingYoThreadNames);
BENCHMARK(LoggingNoThreadNames);
BENCHMARK(LoggingYoCategory);
BENCHMARK(LoggingNoCategory);
BENCHMARK(LoggingNoFile);
