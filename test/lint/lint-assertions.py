#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Copyright (c) 2010-2024 The Freicoin Developers
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
# Check for assertions with obvious side effects.

import sys
import subprocess


def git_grep(params: [], error_msg: ""):
    try:
        output = subprocess.check_output(["git", "grep", *params], text=True, encoding="utf8")
        print(error_msg)
        print(output)
        return 1
    except subprocess.CalledProcessError as ex1:
        if ex1.returncode > 1:
            raise ex1
    return 0


def main():
    # Aborting the whole process is undesirable for RPC code. So nonfatal
    # checks should be used over assert. See: src/util/check.h
    # src/rpc/server.cpp is excluded from this check since it's mostly meta-code.
    exit_code = git_grep([
        "-nE",
        r"\<(A|a)ss(ume|ert) *\(.*\);",
        "--",
        "src/rpc/",
        "src/wallet/rpc*",
        ":(exclude)src/rpc/server.cpp",
    ], "CHECK_NONFATAL(condition) or NONFATAL_UNREACHABLE should be used instead of assert for RPC code.")

    # The `BOOST_ASSERT` macro requires to `#include boost/assert.hpp`,
    # which is an unnecessary Boost dependency.
    exit_code |= git_grep([
        "-E",
        r"BOOST_ASSERT *\(.*\);",
        "--",
        "*.cpp",
        "*.h",
    ], "BOOST_ASSERT must be replaced with Assert, BOOST_REQUIRE, or BOOST_CHECK.")

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
