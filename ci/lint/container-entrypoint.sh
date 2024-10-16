#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
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

export LC_ALL=C

# Fixes permission issues when there is a container UID/GID mismatch with the owner
# of the mounted freicoin src dir.
git config --global --add safe.directory /freicoin

export PATH="/python_build/bin:${PATH}"
export LINT_RUNNER_PATH="/lint_test_runner"

if [ -z "$1" ]; then
  bash -ic "./ci/lint/06_script.sh"
else
  exec "$@"
fi
