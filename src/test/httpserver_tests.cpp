// Copyright (c) 2012-2022 The Bitcoin Core developers
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

#include <httpserver.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(httpserver_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_query_parameters)
{
    std::string uri {};

    // No parameters
    uri = "localhost:8080/rest/headers/someresource.json";
    BOOST_CHECK(!GetQueryParameterFromUri(uri.c_str(), "p1").has_value());

    // Single parameter
    uri = "localhost:8080/rest/endpoint/someresource.json?p1=v1";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri.c_str(), "p1").value(), "v1");
    BOOST_CHECK(!GetQueryParameterFromUri(uri.c_str(), "p2").has_value());

    // Multiple parameters
    uri = "/rest/endpoint/someresource.json?p1=v1&p2=v2";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri.c_str(), "p1").value(), "v1");
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri.c_str(), "p2").value(), "v2");

    // If the query string contains duplicate keys, the first value is returned
    uri = "/rest/endpoint/someresource.json?p1=v1&p1=v2";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri.c_str(), "p1").value(), "v1");

    // Invalid query string syntax is the same as not having parameters
    uri = "/rest/endpoint/someresource.json&p1=v1&p2=v2";
    BOOST_CHECK(!GetQueryParameterFromUri(uri.c_str(), "p1").has_value());

    // URI with invalid characters (%) raises a runtime error regardless of which query parameter is queried
    uri = "/rest/endpoint/someresource.json&p1=v1&p2=v2%";
    BOOST_CHECK_EXCEPTION(GetQueryParameterFromUri(uri.c_str(), "p1"), std::runtime_error, HasReason("URI parsing failed, it likely contained RFC 3986 invalid characters"));
}
BOOST_AUTO_TEST_SUITE_END()
