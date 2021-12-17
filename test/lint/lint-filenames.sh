#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Copyright (c) 2010-2021 The Freicoin Developers
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
# Make sure only lowercase alphanumerics (a-z0-9), underscores (_),
# hyphens (-) and dots (.) are used in source code filenames.

export LC_ALL=C

EXIT_CODE=0
OUTPUT=$(git ls-files --full-name -- "*.[cC][pP][pP]" "*.[hH]" "*.[pP][yY]" "*.[sS][hH]" | \
    grep -vE '^[a-z0-9_./-]+$' | \
    grep -vE '^src/(secp256k1/|univalue/|test/fuzz/FuzzedDataProvider.h)')

if [[ ${OUTPUT} != "" ]]; then
    echo "Use only lowercase alphanumerics (a-z0-9), underscores (_), hyphens (-) and dots (.)"
    echo "in source code filenames:"
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi
exit ${EXIT_CODE}
