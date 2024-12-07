// Copyright (c) 2024-present The Bitcoin Core developers
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
//

#include <node/warnings.h>
#include <util/translation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(node_warnings_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(warnings)
{
    node::Warnings warnings;
    // On pre-release builds, a warning is generated automatically
    warnings.Unset(node::Warning::PRE_RELEASE_TEST_BUILD);

    // For these tests, we don't care what the exact warnings are, so
    // just refer to them as warning_1 and warning_2
    const auto warning_1{node::Warning::CLOCK_OUT_OF_SYNC};
    const auto warning_2{node::Warning::FATAL_INTERNAL_ERROR};

    // Ensure we start without any warnings
    BOOST_CHECK(warnings.GetMessages().size() == 0);
    // Add two warnings
    BOOST_CHECK(warnings.Set(warning_1, _("warning 1")));
    BOOST_CHECK(warnings.Set(warning_2, _("warning 2")));
    // Unset the second one
    BOOST_CHECK(warnings.Unset(warning_2));
    // Since it's already been unset, this should return false
    BOOST_CHECK(!warnings.Unset(warning_2));
    // We should now be able to set w2 again
    BOOST_CHECK(warnings.Set(warning_2, _("warning 2 - revision 1")));
    // Setting w2 again should return false since it's already set
    BOOST_CHECK(!warnings.Set(warning_2, _("warning 2 - revision 2")));

    // Verify messages are correct
    const auto messages{warnings.GetMessages()};
    BOOST_CHECK(messages.size() == 2);
    BOOST_CHECK(messages[0].original == "warning 1");
    BOOST_CHECK(messages[1].original == "warning 2 - revision 1");

    // Clearing all warnings should also clear all messages
    BOOST_CHECK(warnings.Unset(warning_1));
    BOOST_CHECK(warnings.Unset(warning_2));
    BOOST_CHECK(warnings.GetMessages().size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
