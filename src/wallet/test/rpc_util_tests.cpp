// Copyright (c) 2022 The Bitcoin Core developers
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

#include <wallet/rpc/util.h>

#include <boost/test/unit_test.hpp>

namespace wallet {

BOOST_AUTO_TEST_SUITE(wallet_util_tests)

BOOST_AUTO_TEST_CASE(util_ParseISO8601DateTime)
{
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("1970-01-01T00:00:00Z"), 0);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("1960-01-01T00:00:00Z"), 0);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2000-01-01T00:00:01Z"), 946684801);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2011-09-30T23:36:17Z"), 1317425777);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2100-12-31T23:59:59Z"), 4133980799);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace wallet
