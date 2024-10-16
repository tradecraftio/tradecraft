// Copyright (c) 2019-2022 The Bitcoin Core developers
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

#include <config/freicoin-config.h> // IWYU pragma: keep
#include <test/util/setup_common.h>
#include <common/run_command.h>
#include <univalue.h>

#ifdef ENABLE_EXTERNAL_SIGNER
#include <util/subprocess.h>
#endif // ENABLE_EXTERNAL_SIGNER

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(system_tests, BasicTestingSetup)

// At least one test is required (in case ENABLE_EXTERNAL_SIGNER is not defined).
// Workaround for https://github.com/bitcoin/bitcoin/issues/19128
BOOST_AUTO_TEST_CASE(dummy)
{
    BOOST_CHECK(true);
}

#ifdef ENABLE_EXTERNAL_SIGNER

BOOST_AUTO_TEST_CASE(run_command)
{
    {
        const UniValue result = RunCommandParseJSON("");
        BOOST_CHECK(result.isNull());
    }
    {
        const UniValue result = RunCommandParseJSON("echo {\"success\": true}");
        BOOST_CHECK(result.isObject());
        const UniValue& success = result.find_value("success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.get_bool(), true);
    }
    {
        // An invalid command is handled by cpp-subprocess
        const std::string expected{"execve failed: "};
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON("invalid_command"), subprocess::CalledProcessError, HasReason(expected));
    }
    {
        // Return non-zero exit code, no output to stderr
        const std::string command{"false"};
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, [&](const std::runtime_error& e) {
            const std::string what{e.what()};
            BOOST_CHECK(what.find(strprintf("RunCommandParseJSON error: process(%s) returned 1: \n", command)) != std::string::npos);
            return true;
        });
    }
    {
        // Return non-zero exit code, with error message for stderr
        const std::string command{"python3 -c 'import sys; print(\"err\", file=sys.stderr); sys.exit(2)'"};
        const std::string expected{"err"};
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, [&](const std::runtime_error& e) {
            const std::string what(e.what());
            BOOST_CHECK(what.find(strprintf("RunCommandParseJSON error: process(%s) returned", command)) != std::string::npos);
            BOOST_CHECK(what.find(expected) != std::string::npos);
            return true;
        });
    }
    {
        // Unable to parse JSON
        const std::string command{"echo {"};
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, HasReason("Unable to parse JSON: {"));
    }
    // Test std::in
    {
        const UniValue result = RunCommandParseJSON("cat", "{\"success\": true}");
        BOOST_CHECK(result.isObject());
        const UniValue& success = result.find_value("success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.get_bool(), true);
    }
}
#endif // ENABLE_EXTERNAL_SIGNER

BOOST_AUTO_TEST_SUITE_END()
