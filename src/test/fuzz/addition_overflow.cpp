// Copyright (c) 2020 The Bitcoin Core developers
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

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

#if defined(__has_builtin)
#if __has_builtin(__builtin_add_overflow)
#define HAVE_BUILTIN_ADD_OVERFLOW
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 5)
#define HAVE_BUILTIN_ADD_OVERFLOW
#endif

namespace {
template <typename T>
void TestAdditionOverflow(FuzzedDataProvider& fuzzed_data_provider)
{
    const T i = fuzzed_data_provider.ConsumeIntegral<T>();
    const T j = fuzzed_data_provider.ConsumeIntegral<T>();
    const bool is_addition_overflow_custom = AdditionOverflow(i, j);
#if defined(HAVE_BUILTIN_ADD_OVERFLOW)
    T result_builtin;
    const bool is_addition_overflow_builtin = __builtin_add_overflow(i, j, &result_builtin);
    assert(is_addition_overflow_custom == is_addition_overflow_builtin);
    if (!is_addition_overflow_custom) {
        assert(i + j == result_builtin);
    }
#else
    if (!is_addition_overflow_custom) {
        (void)(i + j);
    }
#endif
}
} // namespace

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    TestAdditionOverflow<int64_t>(fuzzed_data_provider);
    TestAdditionOverflow<uint64_t>(fuzzed_data_provider);
    TestAdditionOverflow<int32_t>(fuzzed_data_provider);
    TestAdditionOverflow<uint32_t>(fuzzed_data_provider);
    TestAdditionOverflow<int16_t>(fuzzed_data_provider);
    TestAdditionOverflow<uint16_t>(fuzzed_data_provider);
    TestAdditionOverflow<char>(fuzzed_data_provider);
    TestAdditionOverflow<unsigned char>(fuzzed_data_provider);
    TestAdditionOverflow<signed char>(fuzzed_data_provider);
}
