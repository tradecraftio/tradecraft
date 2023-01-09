#!/usr/bin/env bash
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
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
# Lint format strings: This program checks that the number of arguments passed
# to a variadic format string function matches the number of format specifiers
# in the format string.

export LC_ALL=C

FUNCTION_NAMES_AND_NUMBER_OF_LEADING_ARGUMENTS=(
    "FatalError,0"
    "fprintf,1"
    "tfm::format,1" # Assuming tfm::::format(std::ostream&, ...
    "LogConnectFailure,1"
    "LogPrint,1"
    "LogPrintf,0"
    "printf,0"
    "snprintf,2"
    "sprintf,1"
    "strprintf,0"
    "vfprintf,1"
    "vprintf,1"
    "vsnprintf,1"
    "vsprintf,1"
    "WalletLogPrintf,0"
)

EXIT_CODE=0
if ! python3 -m doctest test/lint/lint-format-strings.py; then
    EXIT_CODE=1
fi
for S in "${FUNCTION_NAMES_AND_NUMBER_OF_LEADING_ARGUMENTS[@]}"; do
    IFS="," read -r FUNCTION_NAME SKIP_ARGUMENTS <<< "${S}"
    for MATCHING_FILE in $(git grep --full-name -l "${FUNCTION_NAME}" -- "*.c" "*.cpp" "*.h" | sort | grep -vE "^src/(leveldb|secp256k1|minisketch|tinyformat|univalue|test/fuzz/strprintf.cpp)"); do
        MATCHING_FILES+=("${MATCHING_FILE}")
    done
    if ! test/lint/lint-format-strings.py --skip-arguments "${SKIP_ARGUMENTS}" "${FUNCTION_NAME}" "${MATCHING_FILES[@]}"; then
        EXIT_CODE=1
    fi
done
exit ${EXIT_CODE}
