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
"""Test successful startup with symlinked directories.
"""

import os

from test_framework.test_framework import FreicoinTestFramework


def rename_and_link(*, from_name, to_name):
    os.rename(from_name, to_name)
    os.symlink(to_name, from_name)
    assert os.path.islink(from_name) and os.path.isdir(from_name)


class SymlinkTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        dir_new_blocks = self.nodes[0].chain_path / "new_blocks"
        dir_new_chainstate = self.nodes[0].chain_path / "new_chainstate"
        self.stop_node(0)

        rename_and_link(
            from_name=self.nodes[0].chain_path / "blocks",
            to_name=dir_new_blocks,
        )
        rename_and_link(
            from_name=self.nodes[0].chain_path / "chainstate",
            to_name=dir_new_chainstate,
        )

        self.start_node(0)


if __name__ == "__main__":
    SymlinkTest().main()
