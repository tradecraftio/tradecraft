#!/usr/bin/env bash
#
# Copyright (c) 2021 The Bitcoin Core developers
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
# Find dead Python code.

export LC_ALL=C

if ! command -v vulture > /dev/null; then
    echo "Skipping Python dead code linting since vulture is not installed. Install by running \"pip3 install vulture\""
    exit 0
fi

# --min-confidence 100 will only report code that is guaranteed to be unused within the analyzed files.
# Any value below 100 introduces the risk of false positives, which would create an unacceptable maintenance burden.
mapfile -t FILES < <(git ls-files -- "*.py")
if ! vulture --min-confidence 100 "${FILES[@]}"; then
    echo "Python dead code detection found some issues"
    exit 1
fi
