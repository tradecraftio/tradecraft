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
# Check for assertions with obvious side effects.

export LC_ALL=C

EXIT_CODE=0

# PRE31-C (SEI CERT C Coding Standard):
# "Assertions should not contain assignments, increment, or decrement operators."
OUTPUT=$(git grep -E '[^_]assert\(.*(\+\+|\-\-|[^=!<>]=[^=!<>]).*\);' -- "*.cpp" "*.h")
if [[ ${OUTPUT} != "" ]]; then
    echo "Assertions should not have side effects:"
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi

# Macro CHECK_NONFATAL(condition) should be used instead of assert for RPC code, where it
# is undesirable to crash the whole program. See: src/util/check.h
# src/rpc/server.cpp is excluded from this check since it's mostly meta-code.
OUTPUT=$(git grep -nE '\<(A|a)ssert *\(.*\);' -- "src/rpc/" "src/wallet/rpc*" ":(exclude)src/rpc/server.cpp")
if [[ ${OUTPUT} != "" ]]; then
    echo "CHECK_NONFATAL(condition) should be used instead of assert for RPC code."
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
