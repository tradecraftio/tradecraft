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
# Check for SIGNAL/SLOT connect style, removed since Qt4 support drop.

export LC_ALL=C

EXIT_CODE=0

OUTPUT=$(git grep -E '(SIGNAL|, ?SLOT)\(' -- src/qt)
if [[ ${OUTPUT} != "" ]]; then
    echo "Use Qt5 connect style in:"
    echo "$OUTPUT"
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
