// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

namespace {
bool IsValidAddition(const CScriptNum& lhs, const CScriptNum& rhs)
{
    return rhs == 0 || (rhs > 0 && lhs <= CScriptNum{std::numeric_limits<int64_t>::max()} - rhs) || (rhs < 0 && lhs >= CScriptNum{std::numeric_limits<int64_t>::min()} - rhs);
}

bool IsValidSubtraction(const CScriptNum& lhs, const CScriptNum& rhs)
{
    return rhs == 0 || (rhs > 0 && lhs >= CScriptNum{std::numeric_limits<int64_t>::min()} + rhs) || (rhs < 0 && lhs <= CScriptNum{std::numeric_limits<int64_t>::max()} + rhs);
}
} // namespace

FUZZ_TARGET(scriptnum_ops)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CScriptNum script_num = ConsumeScriptNum(fuzzed_data_provider);
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 1000000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const int64_t i = fuzzed_data_provider.ConsumeIntegral<int64_t>();
                assert((script_num == i) != (script_num != i));
                assert((script_num <= i) != (script_num > i));
                assert((script_num >= i) != (script_num < i));
                // Avoid signed integer overflow:
                // script/script.h:264:93: runtime error: signed integer overflow: -2261405121394637306 + -9223372036854775802 cannot be represented in type 'long'
                if (IsValidAddition(script_num, CScriptNum{i})) {
                    assert((script_num + i) - i == script_num);
                }
                // Avoid signed integer overflow:
                // script/script.h:265:93: runtime error: signed integer overflow: 9223371895120855039 - -9223372036854710486 cannot be represented in type 'long'
                if (IsValidSubtraction(script_num, CScriptNum{i})) {
                    assert((script_num - i) + i == script_num);
                }
            },
            [&] {
                const CScriptNum random_script_num = ConsumeScriptNum(fuzzed_data_provider);
                assert((script_num == random_script_num) != (script_num != random_script_num));
                assert((script_num <= random_script_num) != (script_num > random_script_num));
                assert((script_num >= random_script_num) != (script_num < random_script_num));
                // Avoid signed integer overflow:
                // script/script.h:264:93: runtime error: signed integer overflow: -9223126527765971126 + -9223372036854756825 cannot be represented in type 'long'
                if (IsValidAddition(script_num, random_script_num)) {
                    assert((script_num + random_script_num) - random_script_num == script_num);
                }
                // Avoid signed integer overflow:
                // script/script.h:265:93: runtime error: signed integer overflow: 6052837899185946624 - -9223372036854775808 cannot be represented in type 'long'
                if (IsValidSubtraction(script_num, random_script_num)) {
                    assert((script_num - random_script_num) + random_script_num == script_num);
                }
            },
            [&] {
                const CScriptNum random_script_num = ConsumeScriptNum(fuzzed_data_provider);
                if (!IsValidAddition(script_num, random_script_num)) {
                    // Avoid assertion failure:
                    // ./script/script.h:292: CScriptNum &CScriptNum::operator+=(const int64_t &): Assertion `rhs == 0 || (rhs > 0 && m_value <= std::numeric_limits<int64_t>::max() - rhs) || (rhs < 0 && m_value >= std::numeric_limits<int64_t>::min() - rhs)' failed.
                    return;
                }
                script_num += random_script_num;
            },
            [&] {
                const CScriptNum random_script_num = ConsumeScriptNum(fuzzed_data_provider);
                if (!IsValidSubtraction(script_num, random_script_num)) {
                    // Avoid assertion failure:
                    // ./script/script.h:300: CScriptNum &CScriptNum::operator-=(const int64_t &): Assertion `rhs == 0 || (rhs > 0 && m_value >= std::numeric_limits<int64_t>::min() + rhs) || (rhs < 0 && m_value <= std::numeric_limits<int64_t>::max() + rhs)' failed.
                    return;
                }
                script_num -= random_script_num;
            },
            [&] {
                script_num = script_num & fuzzed_data_provider.ConsumeIntegral<int64_t>();
            },
            [&] {
                script_num = script_num & ConsumeScriptNum(fuzzed_data_provider);
            },
            [&] {
                script_num &= ConsumeScriptNum(fuzzed_data_provider);
            },
            [&] {
                if (script_num == CScriptNum{std::numeric_limits<int64_t>::min()}) {
                    // Avoid assertion failure:
                    // ./script/script.h:279: CScriptNum CScriptNum::operator-() const: Assertion `m_value != std::numeric_limits<int64_t>::min()' failed.
                    return;
                }
                script_num = -script_num;
            },
            [&] {
                script_num = fuzzed_data_provider.ConsumeIntegral<int64_t>();
            },
            [&] {
                const int64_t random_integer = fuzzed_data_provider.ConsumeIntegral<int64_t>();
                if (!IsValidAddition(script_num, CScriptNum{random_integer})) {
                    // Avoid assertion failure:
                    // ./script/script.h:292: CScriptNum &CScriptNum::operator+=(const int64_t &): Assertion `rhs == 0 || (rhs > 0 && m_value <= std::numeric_limits<int64_t>::max() - rhs) || (rhs < 0 && m_value >= std::numeric_limits<int64_t>::min() - rhs)' failed.
                    return;
                }
                script_num += random_integer;
            },
            [&] {
                const int64_t random_integer = fuzzed_data_provider.ConsumeIntegral<int64_t>();
                if (!IsValidSubtraction(script_num, CScriptNum{random_integer})) {
                    // Avoid assertion failure:
                    // ./script/script.h:300: CScriptNum &CScriptNum::operator-=(const int64_t &): Assertion `rhs == 0 || (rhs > 0 && m_value >= std::numeric_limits<int64_t>::min() + rhs) || (rhs < 0 && m_value <= std::numeric_limits<int64_t>::max() + rhs)' failed.
                    return;
                }
                script_num -= random_integer;
            },
            [&] {
                script_num &= fuzzed_data_provider.ConsumeIntegral<int64_t>();
            });
        (void)script_num.getint();
        (void)script_num.getvch();
    }
}
