#!/usr/bin/env python3
#
# Copyright (c) 2022 The Bitcoin Core developers
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

"""
This script checks for git modules
"""

import subprocess
import sys

def main():
    submodules_list = subprocess.check_output(['git', 'submodule', 'status', '--recursive'],
                                                universal_newlines = True, encoding = 'utf8').rstrip('\n')
    if submodules_list:
        print("These submodules were found, delete them:\n", submodules_list)
        sys.exit(1)
    sys.exit(0)

if __name__ == '__main__':
    main()
