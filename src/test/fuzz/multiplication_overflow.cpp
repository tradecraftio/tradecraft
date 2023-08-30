// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#if defined(HAVE_CONFIG_H)
#include <config/freicoin-config.h>
#endif

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
template <typename T>
void TestMultiplicationOverflow(FuzzedDataProvider& fuzzed_data_provider)
{
    const T i = fuzzed_data_provider.ConsumeIntegral<T>();
    const T j = fuzzed_data_provider.ConsumeIntegral<T>();
    const bool is_multiplication_overflow_custom = MultiplicationOverflow(i, j);
#if defined(HAVE_BUILTIN_MUL_OVERFLOW)
    T result_builtin;
    const bool is_multiplication_overflow_builtin = __builtin_mul_overflow(i, j, &result_builtin);
    assert(is_multiplication_overflow_custom == is_multiplication_overflow_builtin);
    if (!is_multiplication_overflow_custom) {
        assert(i * j == result_builtin);
    }
#else
    if (!is_multiplication_overflow_custom) {
        (void)(i * j);
    }
#endif
}
} // namespace

FUZZ_TARGET(multiplication_overflow)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    TestMultiplicationOverflow<int64_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint64_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<int32_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint32_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<int16_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint16_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<char>(fuzzed_data_provider);
    TestMultiplicationOverflow<unsigned char>(fuzzed_data_provider);
    TestMultiplicationOverflow<signed char>(fuzzed_data_provider);
}
