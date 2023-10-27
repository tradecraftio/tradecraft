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
# Make sure we explicitly open all text files using UTF-8 (or ASCII) encoding to
# avoid potential issues on the BSDs where the locale is not always set.

export LC_ALL=C
EXIT_CODE=0
OUTPUT=$(git grep " open(" -- "*.py" ":(exclude)src/crc32c/" | grep -vE "encoding=.(ascii|utf8|utf-8)." | grep -vE "open\([^,]*, ['\"][^'\"]*b[^'\"]*['\"]")
if [[ ${OUTPUT} != "" ]]; then
    echo "Python's open(...) seems to be used to open text files without explicitly"
    echo "specifying encoding=\"utf8\":"
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi
OUTPUT=$(git grep "check_output(" -- "*.py" ":(exclude)src/crc32c/"| grep "universal_newlines=True" | grep -vE "encoding=.(ascii|utf8|utf-8).")
if [[ ${OUTPUT} != "" ]]; then
    echo "Python's check_output(...) seems to be used to get program outputs without explicitly"
    echo "specifying encoding=\"utf8\":"
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi
exit ${EXIT_CODE}
