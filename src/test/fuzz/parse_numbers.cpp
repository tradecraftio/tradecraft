// Copyright (c) 2009-2019 The Bitcoin Core developers
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

#include <test/fuzz/fuzz.h>
#include <util/moneystr.h>
#include <util/strencodings.h>

#include <string>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string random_string(buffer.begin(), buffer.end());

    CAmount amount;
    (void)ParseMoney(random_string, amount);

    double d;
    (void)ParseDouble(random_string, &d);

    int32_t i32;
    (void)ParseInt32(random_string, &i32);
    (void)atoi(random_string);

    uint32_t u32;
    (void)ParseUInt32(random_string, &u32);

    int64_t i64;
    (void)atoi64(random_string);
    (void)ParseFixedPoint(random_string, 3, &i64);
    (void)ParseInt64(random_string, &i64);

    uint64_t u64;
    (void)ParseUInt64(random_string, &u64);
}
