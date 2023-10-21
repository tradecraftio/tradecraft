// Copyright (c) 2023 The Bitcoin Core developers
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

#include <tinyformat.h>
#include <util/translation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(translation_tests)

BOOST_AUTO_TEST_CASE(translation_namedparams)
{
    bilingual_str arg{"original", "translated"};
    bilingual_str format{"original [%s]", "translated [%s]"};
    bilingual_str result{strprintf(format, arg)};
    BOOST_CHECK_EQUAL(result.original, "original [original]");
    BOOST_CHECK_EQUAL(result.translated, "translated [translated]");
}

BOOST_AUTO_TEST_SUITE_END()
