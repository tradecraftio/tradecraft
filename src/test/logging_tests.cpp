// Copyright (c) 2019-2021 The Bitcoin Core developers
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

#include <logging.h>
#include <logging/timer.h>
#include <test/util/setup_common.h>

#include <chrono>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(logging_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(logging_timer)
{
    SetMockTime(1);
    auto micro_timer = BCLog::Timer<std::chrono::microseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(micro_timer.LogMsg("test micros"), "tests: test micros (1000000μs)");

    SetMockTime(1);
    auto ms_timer = BCLog::Timer<std::chrono::milliseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(ms_timer.LogMsg("test ms"), "tests: test ms (1000.00ms)");

    SetMockTime(1);
    auto sec_timer = BCLog::Timer<std::chrono::seconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(sec_timer.LogMsg("test secs"), "tests: test secs (1.00s)");
}

BOOST_AUTO_TEST_SUITE_END()
