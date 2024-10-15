// Copyright (c) 2024- The Bitcoin Core developers
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
#include <random.h>
#include <stddef.h>
#include <util/strencodings.h>
#include <cassert>
#include <optional>
#include <vector>

std::string generateHexString(size_t length) {
    const auto hex_digits = "0123456789ABCDEF";
    FastRandomContext rng(/*fDeterministic=*/true);

    std::string data;
    while (data.size() < length) {
        auto digit = hex_digits[rng.randbits(4)];
        data.push_back(digit);
    }
    return data;
}

static void HexParse(benchmark::Bench& bench)
{
    auto data = generateHexString(130); // Generates 678B0EDA0A1FD30904D5A65E3568DB82DB2D918B0AD8DEA18A63FECCB877D07CAD1495C7157584D877420EF38B8DA473A6348B4F51811AC13C786B962BEE5668F9 by default

    bench.batch(data.size()).unit("base16").run([&] {
        auto result = TryParseHex(data);
        assert(result != std::nullopt); // make sure we're measuring the successful case
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

BENCHMARK(HexParse, benchmark::PriorityLevel::HIGH);
