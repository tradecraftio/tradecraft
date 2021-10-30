#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
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
# Check for shellcheck warnings in shell scripts.

export LC_ALL=C

# The shellcheck binary segfault/coredumps in Travis with LC_ALL=C
# It does not do so in Ubuntu 14.04, 16.04, 18.04 in versions 0.3.3, 0.3.7, 0.4.6
# respectively. So export LC_ALL=C is set as required by lint-shell-locale.sh
# but unset here in case of running in Travis.
if [ "$TRAVIS" = "true" ]; then
    unset LC_ALL
fi

if ! command -v shellcheck > /dev/null; then
    echo "Skipping shell linting since shellcheck is not installed."
    exit 0
fi

# Disabled warnings:
disabled=(
    SC2046 # Quote this to prevent word splitting.
    SC2086 # Double quote to prevent globbing and word splitting.
    SC2162 # read without -r will mangle backslashes.
)
shellcheck -e "$(IFS=","; echo "${disabled[*]}")" \
    $(git ls-files -- "*.sh" | grep -vE 'src/(secp256k1|univalue)/')
