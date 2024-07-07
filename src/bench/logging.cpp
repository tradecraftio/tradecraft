// Copyright (c) 2020-2022 The Bitcoin Core developers
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
#include <logging.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

// All but 2 of the benchmarks should have roughly similar performance:
//
// LogPrintWithoutCategory should be ~3 orders of magnitude faster, as nothing is logged.
//
// LogWithoutWriteToFile should be ~2 orders of magnitude faster, as it avoids disk writes.

static void Logging(benchmark::Bench& bench, const std::vector<const char*>& extra_args, const std::function<void()>& log)
{
    // Reset any enabled logging categories from a previous benchmark run.
    LogInstance().DisableCategory(BCLog::LogFlags::ALL);

    TestingSetup test_setup{
        ChainType::REGTEST,
        extra_args,
    };

    bench.run([&] { log(); });
}

static void LogPrintLevelWithThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1", "-debug=net"}, [] {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", "test"); });
}

static void LogPrintLevelWithoutThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=net"}, [] {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", "test"); });
}

static void LogPrintWithCategory(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=net"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}

static void LogPrintWithoutCategory(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=0"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}

static void LogPrintfCategoryWithThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1", "-debug=net"}, [] {
        LogPrintfCategory(BCLog::NET, "%s\n", "test");
    });
}

static void LogPrintfCategoryWithoutThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=net"}, [] {
        LogPrintfCategory(BCLog::NET, "%s\n", "test");
    });
}

static void LogPrintfWithThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1"}, [] { LogPrintf("%s\n", "test"); });
}

static void LogPrintfWithoutThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0"}, [] { LogPrintf("%s\n", "test"); });
}

static void LogWithoutWriteToFile(benchmark::Bench& bench)
{
    // Disable writing the log to a file, as used for unit tests and fuzzing in `MakeNoLogFileContext`.
    Logging(bench, {"-nodebuglogfile", "-debug=1"}, [] {
        LogPrintf("%s\n", "test");
        LogPrint(BCLog::NET, "%s\n", "test");
    });
}

BENCHMARK(LogPrintLevelWithThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogPrintLevelWithoutThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogPrintWithCategory, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogPrintWithoutCategory, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogPrintfCategoryWithThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogPrintfCategoryWithoutThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogPrintfWithThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogPrintfWithoutThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogWithoutWriteToFile, benchmark::PriorityLevel::HIGH);
