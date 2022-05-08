// Copyright (c) 2020 The Bitcoin Core developers
// Copyright (c) 2011-2022 The Freicoin Developers
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

#include <util/ref.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ref_tests)

BOOST_AUTO_TEST_CASE(ref_test)
{
    util::Ref ref;
    BOOST_CHECK(!ref.Has<int>());
    BOOST_CHECK_THROW(ref.Get<int>(), NonFatalCheckError);
    int value = 5;
    ref.Set(value);
    BOOST_CHECK(ref.Has<int>());
    BOOST_CHECK_EQUAL(ref.Get<int>(), 5);
    ++ref.Get<int>();
    BOOST_CHECK_EQUAL(ref.Get<int>(), 6);
    BOOST_CHECK_EQUAL(value, 6);
    ++value;
    BOOST_CHECK_EQUAL(value, 7);
    BOOST_CHECK_EQUAL(ref.Get<int>(), 7);
    BOOST_CHECK(!ref.Has<bool>());
    BOOST_CHECK_THROW(ref.Get<bool>(), NonFatalCheckError);
    ref.Clear();
    BOOST_CHECK(!ref.Has<int>());
    BOOST_CHECK_THROW(ref.Get<int>(), NonFatalCheckError);
}

BOOST_AUTO_TEST_SUITE_END()
