#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Copyright (c) 2010-2023 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of version 3 of the GNU Affero General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
# details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Check the test suite naming conventions

export LC_ALL=C
EXIT_CODE=0

NAMING_INCONSISTENCIES=$(git grep -E '^BOOST_FIXTURE_TEST_SUITE\(' -- \
    "src/test/**.cpp" "src/wallet/test/**.cpp" | \
    grep -vE '/(.*?)\.cpp:BOOST_FIXTURE_TEST_SUITE\(\1, .*\)$')
if [[ ${NAMING_INCONSISTENCIES} != "" ]]; then
    echo "The test suite in file src/test/foo_tests.cpp should be named"
    echo "\"foo_tests\". Please make sure the following test suites follow"
    echo "that convention:"
    echo
    echo "${NAMING_INCONSISTENCIES}"
    EXIT_CODE=1
fi

TEST_SUITE_NAME_COLLISSIONS=$(git grep -E '^BOOST_FIXTURE_TEST_SUITE\(' -- \
    "src/test/**.cpp" "src/wallet/test/**.cpp" | cut -f2 -d'(' | cut -f1 -d, | \
    sort | uniq -d)
if [[ ${TEST_SUITE_NAME_COLLISSIONS} != "" ]]; then
    echo "Test suite names must be unique. The following test suite names"
    echo "appear to be used more than once:"
    echo
    echo "${TEST_SUITE_NAME_COLLISSIONS}"
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
