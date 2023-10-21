#!/usr/bin/env bash
#
# Copyright (c) 2014-2021 The Bitcoin Core developers
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

export LC_ALL=C
FRAMEDIR=$(dirname "$0")
for i in {0..35}
do
    frame=$(printf "%03d" "$i")
    angle=$((i * 10))
    convert "${FRAMEDIR}/../src/spinner.png" -background "rgba(0,0,0,0.0)" -distort SRT $angle "${FRAMEDIR}/spinner-${frame}.png"
done
