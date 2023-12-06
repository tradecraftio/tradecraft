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

import shutil
from pathlib import Path

from test_framework.test_framework import FreicoinTestFramework, initialize_datadir


class BlocksdirTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        self.stop_node(0)
        assert self.nodes[0].blocks_path.is_dir()
        assert not (self.nodes[0].datadir_path / "blocks").is_dir()
        shutil.rmtree(self.nodes[0].datadir_path)
        initialize_datadir(self.options.tmpdir, 0, self.chain)
        self.log.info("Starting with nonexistent blocksdir ...")
        blocksdir_path = Path(self.options.tmpdir) / 'blocksdir'
        self.nodes[0].assert_start_raises_init_error([f"-blocksdir={blocksdir_path}"], f'Error: Specified blocks directory "{blocksdir_path}" does not exist.')
        blocksdir_path.mkdir()
        self.log.info("Starting with existing blocksdir ...")
        self.start_node(0, [f"-blocksdir={blocksdir_path}"])
        self.log.info("mining blocks..")
        self.generatetoaddress(self.nodes[0], 10, self.nodes[0].get_deterministic_priv_key().address)
        assert (blocksdir_path / self.chain / "blocks" / "blk00000.dat").is_file()
        assert (self.nodes[0].blocks_path / "index").is_dir()


if __name__ == '__main__':
    BlocksdirTest().main()
