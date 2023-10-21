#!/usr/bin/env python3
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
"""Test file system permissions for POSIX platforms.
"""

import os
import stat

from test_framework.test_framework import BitcoinTestFramework


class PosixFsPermissionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_posix()

    def check_directory_permissions(self, dir):
        mode = os.lstat(dir).st_mode
        self.log.info(f"{stat.filemode(mode)} {dir}")
        assert mode == (stat.S_IFDIR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)

    def check_file_permissions(self, file):
        mode = os.lstat(file).st_mode
        self.log.info(f"{stat.filemode(mode)} {file}")
        assert mode == (stat.S_IFREG | stat.S_IRUSR | stat.S_IWUSR)

    def run_test(self):
        self.stop_node(0)
        datadir = os.path.join(self.nodes[0].datadir, self.chain)
        self.check_directory_permissions(datadir)
        walletsdir = os.path.join(datadir, "wallets")
        self.check_directory_permissions(walletsdir)
        debuglog = os.path.join(datadir, "debug.log")
        self.check_file_permissions(debuglog)


if __name__ == '__main__':
    PosixFsPermissionsTest().main()
