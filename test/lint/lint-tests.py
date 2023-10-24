#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
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

"""
Check the test suite naming conventions
"""

import re
import subprocess
import sys


def grep_boost_fixture_test_suite():
    command = [
        "git",
        "grep",
        "-E",
        r"^BOOST_FIXTURE_TEST_SUITE\(",
        "--",
        "src/test/**.cpp",
        "src/wallet/test/**.cpp",
    ]
    return subprocess.check_output(command, universal_newlines=True, encoding="utf8")


def check_matching_test_names(test_suite_list):
    not_matching = [
        x
        for x in test_suite_list
        if re.search(r"/(.*?)\.cpp:BOOST_FIXTURE_TEST_SUITE\(\1, .*\)", x) is None
    ]
    if len(not_matching) > 0:
        not_matching = "\n".join(not_matching)
        error_msg = (
            "The test suite in file src/test/foo_tests.cpp should be named\n"
            '"foo_tests". Please make sure the following test suites follow\n'
            "that convention:\n\n"
            f"{not_matching}\n"
        )
        print(error_msg)
        return 1
    return 0


def get_duplicates(input_list):
    """
    From https://stackoverflow.com/a/9835819
    """
    seen = set()
    dupes = set()
    for x in input_list:
        if x in seen:
            dupes.add(x)
        else:
            seen.add(x)
    return dupes


def check_unique_test_names(test_suite_list):
    output = [re.search(r"\((.*?),", x) for x in test_suite_list]
    output = [x.group(1) for x in output if x is not None]
    output = get_duplicates(output)
    output = sorted(list(output))

    if len(output) > 0:
        output = "\n".join(output)
        error_msg = (
            "Test suite names must be unique. The following test suite names\n"
            f"appear to be used more than once:\n\n{output}"
        )
        print(error_msg)
        return 1
    return 0


def main():
    test_suite_list = grep_boost_fixture_test_suite().splitlines()
    exit_code = check_matching_test_names(test_suite_list)
    exit_code |= check_unique_test_names(test_suite_list)
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
