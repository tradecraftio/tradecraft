#!/usr/bin/env python3
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
"""Test the blocksdir option.
"""

import os
import shutil

from test_framework.test_framework import FreicoinTestFramework, initialize_datadir


class BlocksdirTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        self.stop_node(0)
        assert os.path.isdir(os.path.join(self.nodes[0].datadir, self.chain, "blocks"))
        assert not os.path.isdir(os.path.join(self.nodes[0].datadir, "blocks"))
        shutil.rmtree(self.nodes[0].datadir)
        initialize_datadir(self.options.tmpdir, 0, self.chain)
        self.log.info("Starting with nonexistent blocksdir ...")
        blocksdir_path = os.path.join(self.options.tmpdir, 'blocksdir')
        self.nodes[0].assert_start_raises_init_error([f"-blocksdir={blocksdir_path}"], f'Error: Specified blocks directory "{blocksdir_path}" does not exist.')
        os.mkdir(blocksdir_path)
        self.log.info("Starting with existing blocksdir ...")
        self.start_node(0, [f"-blocksdir={blocksdir_path}"])
        self.log.info("mining blocks..")
        self.generatetoaddress(self.nodes[0], 10, self.nodes[0].get_deterministic_priv_key().address)
        assert os.path.isfile(os.path.join(blocksdir_path, self.chain, "blocks", "blk00000.dat"))
        assert os.path.isdir(os.path.join(self.nodes[0].datadir, self.chain, "blocks", "index"))


if __name__ == '__main__':
    BlocksdirTest().main()
