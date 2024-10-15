#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Copyright (c) 2010-2024 The Freicoin Developers
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

export LC_ALL=C.UTF-8

# Only used in .cirrus.yml. Refer to test/lint/README.md on how to run locally.

cp "./ci/retry/retry" "/ci_retry"
cp "./.python-version" "/.python-version"
mkdir --parents "/test/lint"
cp --recursive "./test/lint/test_runner" "/test/lint/"
set -o errexit; source ./ci/lint/04_install.sh
set -o errexit
./ci/lint/06_script.sh
