#!/usr/bin/env python3
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
'''
Common utility functions
'''
import shutil
import sys
import os
from typing import List


def determine_wellknown_cmd(envvar, progname) -> List[str]:
    maybe_env = os.getenv(envvar)
    maybe_which = shutil.which(progname)
    if maybe_env:
        return maybe_env.split(' ')  # Well-known vars are often meant to be word-split
    elif maybe_which:
        return [ maybe_which ]
    else:
        sys.exit(f"{progname} not found")
