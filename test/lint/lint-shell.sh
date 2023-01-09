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
# Check for shellcheck warnings in shell scripts.

export LC_ALL=C

# Disabled warnings:
disabled=(
    SC2162 # read without -r will mangle backslashes.
)

EXIT_CODE=0

if ! command -v shellcheck > /dev/null; then
    echo "Skipping shell linting since shellcheck is not installed."
    exit $EXIT_CODE
fi

SHELLCHECK_CMD=(shellcheck --external-sources --check-sourced --source-path=SCRIPTDIR)
EXCLUDE="--exclude=$(IFS=','; echo "${disabled[*]}")"
# Check shellcheck directive used for sourced files
mapfile -t SOURCED_FILES < <(git ls-files | xargs gawk '/^# shellcheck shell=/ {print FILENAME} {nextfile}')
mapfile -t GUIX_FILES < <(git ls-files contrib/guix contrib/shell | xargs gawk '/^#!\/usr\/bin\/env bash/ {print FILENAME} {nextfile}')
mapfile -t FILES < <(git ls-files -- '*.sh' | grep -vE 'src/(leveldb|secp256k1|minisketch|univalue)/')
if ! "${SHELLCHECK_CMD[@]}" "$EXCLUDE" "${SOURCED_FILES[@]}" "${GUIX_FILES[@]}" "${FILES[@]}"; then
    EXIT_CODE=1
fi

exit $EXIT_CODE
