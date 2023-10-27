// Copyright (c) 2016-2021 The Bitcoin Core developers
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

#include <compat/byteswap.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bswap_tests)

BOOST_AUTO_TEST_CASE(bswap_tests)
{
    uint16_t u1 = 0x1234;
    uint32_t u2 = 0x56789abc;
    uint64_t u3 = 0xdef0123456789abc;
    uint16_t e1 = 0x3412;
    uint32_t e2 = 0xbc9a7856;
    uint64_t e3 = 0xbc9a78563412f0de;
    BOOST_CHECK(bswap_16(u1) == e1);
    BOOST_CHECK(bswap_32(u2) == e2);
    BOOST_CHECK(bswap_64(u3) == e3);
}

BOOST_AUTO_TEST_SUITE_END()
