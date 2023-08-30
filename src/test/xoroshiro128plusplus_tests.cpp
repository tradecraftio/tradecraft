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

#include <test/util/setup_common.h>
#include <test/util/xoroshiro128plusplus.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(xoroshiro128plusplus_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(reference_values)
{
    // numbers generated from reference implementation
    XoRoShiRo128PlusPlus rng(0);
    BOOST_TEST(0x6f68e1e7e2646ee1 == rng());
    BOOST_TEST(0xbf971b7f454094ad == rng());
    BOOST_TEST(0x48f2de556f30de38 == rng());
    BOOST_TEST(0x6ea7c59f89bbfc75 == rng());

    // seed with a random number
    rng = XoRoShiRo128PlusPlus(0x1a26f3fa8546b47a);
    BOOST_TEST(0xc8dc5e08d844ac7d == rng());
    BOOST_TEST(0x5b5f1f6d499dad1b == rng());
    BOOST_TEST(0xbeb0031f93313d6f == rng());
    BOOST_TEST(0xbfbcf4f43a264497 == rng());
}

BOOST_AUTO_TEST_SUITE_END()
