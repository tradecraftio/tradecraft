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

#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(span)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    std::string str = fuzzed_data_provider.ConsumeBytesAsString(32);
    const Span<const char> span{str};
    (void)span.data();
    (void)span.begin();
    (void)span.end();
    if (span.size() > 0) {
        const std::ptrdiff_t idx = fuzzed_data_provider.ConsumeIntegralInRange<std::ptrdiff_t>(0U, span.size() - 1U);
        (void)span.first(idx);
        (void)span.last(idx);
        (void)span.subspan(idx);
        (void)span.subspan(idx, span.size() - idx);
        (void)span[idx];
    }

    std::string another_str = fuzzed_data_provider.ConsumeBytesAsString(32);
    const Span<const char> another_span{another_str};
    assert((span <= another_span) != (span > another_span));
    assert((span == another_span) != (span != another_span));
    assert((span >= another_span) != (span < another_span));
}
