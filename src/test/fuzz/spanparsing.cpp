// Copyright (c) 2019-2020 The Bitcoin Core developers
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

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/spanparsing.h>

FUZZ_TARGET(spanparsing)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const size_t query_size = fuzzed_data_provider.ConsumeIntegral<size_t>();
    const std::string query = fuzzed_data_provider.ConsumeBytesAsString(std::min<size_t>(query_size, 1024 * 1024));
    const std::string span_str = fuzzed_data_provider.ConsumeRemainingBytesAsString();
    const Span<const char> const_span{span_str};

    Span<const char> mut_span = const_span;
    (void)spanparsing::Const(query, mut_span);

    mut_span = const_span;
    (void)spanparsing::Func(query, mut_span);

    mut_span = const_span;
    (void)spanparsing::Expr(mut_span);

    if (!query.empty()) {
        mut_span = const_span;
        (void)spanparsing::Split(mut_span, query.front());
    }
}
