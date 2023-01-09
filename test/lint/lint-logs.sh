#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
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
# Check that all logs are terminated with '\n'
#
# Some logs are continued over multiple lines. They should be explicitly
# commented with /* Continued */
#
# There are some instances of LogPrintf() in comments. Those can be
# ignored

export LC_ALL=C
UNTERMINATED_LOGS=$(git grep --extended-regexp "LogPrintf?\(" -- "*.cpp" | \
    grep -v '\\n"' | \
    grep -v '\.\.\.' | \
    grep -v "/\* Continued \*/" | \
    grep -v "LogPrint()" | \
    grep -v "LogPrintf()")
if [[ ${UNTERMINATED_LOGS} != "" ]]; then
    # shellcheck disable=SC2028
    echo "All calls to LogPrintf() and LogPrint() should be terminated with \\n"
    echo
    echo "${UNTERMINATED_LOGS}"
    exit 1
fi
