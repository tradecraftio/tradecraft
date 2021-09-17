#!/usr/bin/env python3
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
"""Test the blocksdir option.
"""

import os
import shutil

from test_framework.test_framework import FreicoinTestFramework, initialize_datadir


class BlocksdirTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.stop_node(0)
        shutil.rmtree(self.nodes[0].datadir)
        initialize_datadir(self.options.tmpdir, 0)
        self.log.info("Starting with non exiting blocksdir ...")
        blocksdir_path = os.path.join(self.options.tmpdir, 'blocksdir')
        self.nodes[0].assert_start_raises_init_error(["-blocksdir=" + blocksdir_path], 'Error: Specified blocks directory "{}" does not exist.'.format(blocksdir_path))
        os.mkdir(blocksdir_path)
        self.log.info("Starting with exiting blocksdir ...")
        self.start_node(0, ["-blocksdir=" + blocksdir_path])
        self.log.info("mining blocks..")
        self.nodes[0].generate(10)
        assert os.path.isfile(os.path.join(blocksdir_path, "regtest", "blocks", "blk00000.dat"))
        assert os.path.isdir(os.path.join(self.nodes[0].datadir, "regtest", "blocks", "index"))


if __name__ == '__main__':
    BlocksdirTest().main()
