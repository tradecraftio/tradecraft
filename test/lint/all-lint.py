#!/usr/bin/env python3
#
# Copyright (c) 2017-2022 The Bitcoin Core developers
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
# This script runs all test/lint/lint-* files, and fails if any exit
# with a non-zero status code.

from glob import glob
from pathlib import Path
from subprocess import run

exit_code = 0
mod_path = Path(__file__).parent
for lint in glob(f"{mod_path}/lint-*.py"):
    result = run([lint])
    if result.returncode != 0:
        print(f"^---- failure generated from {lint.split('/')[-1]}")
        exit_code |= result.returncode

exit(exit_code)
