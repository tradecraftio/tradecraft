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
# Check include guards.

export LC_ALL=C
HEADER_ID_PREFIX="BITCOIN_"
HEADER_ID_SUFFIX="_H"

REGEXP_EXCLUDE_FILES_WITH_PREFIX="src/(crypto/ctaes/|leveldb/|crc32c/|secp256k1/|minisketch/|test/fuzz/FuzzedDataProvider.h|tinyformat.h|bench/nanobench.h|univalue/)"

EXIT_CODE=0
for HEADER_FILE in $(git ls-files -- "*.h" | grep -vE "^${REGEXP_EXCLUDE_FILES_WITH_PREFIX}")
do
    HEADER_ID_BASE=$(cut -f2- -d/ <<< "${HEADER_FILE}" | sed "s/\.h$//g" | tr / _ | tr - _ | tr "[:lower:]" "[:upper:]")
    HEADER_ID="${HEADER_ID_PREFIX}${HEADER_ID_BASE}${HEADER_ID_SUFFIX}"
    if [[ $(grep --count --extended-regexp "^#(ifndef|define|endif //) ${HEADER_ID}" "${HEADER_FILE}") != 3 ]]; then
        echo "${HEADER_FILE} seems to be missing the expected include guard:"
        echo "  #ifndef ${HEADER_ID}"
        echo "  #define ${HEADER_ID}"
        echo "  ..."
        echo "  #endif // ${HEADER_ID}"
        echo
        EXIT_CODE=1
    fi
done
exit ${EXIT_CODE}
