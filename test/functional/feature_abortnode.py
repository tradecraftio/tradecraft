#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
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
"""Test freicoind aborts if can't disconnect a block.

- Start a single node and generate 3 blocks.
- Delete the undo data.
- Mine a fork that requires disconnecting the tip.
- Verify that freicoind AbortNode's.
"""

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import wait_until, get_datadir_path, connect_nodes
import os

class AbortNodeTest(FreicoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        self.setup_nodes()
        # We'll connect the nodes later

    def run_test(self):
        self.nodes[0].generate(3)
        datadir = get_datadir_path(self.options.tmpdir, 0)

        # Deleting the undo file will result in reorg failure
        os.unlink(os.path.join(datadir, 'regtest', 'blocks', 'rev00000.dat'))

        # Connecting to a node with a more work chain will trigger a reorg
        # attempt.
        self.nodes[1].generate(3)
        with self.nodes[0].assert_debug_log(["Failed to disconnect block"]):
            connect_nodes(self.nodes[0], 1)
            self.nodes[1].generate(1)

            # Check that node0 aborted
            self.log.info("Waiting for crash")
            wait_until(lambda: self.nodes[0].is_node_stopped(), timeout=60)
        self.log.info("Node crashed - now verifying restart fails")
        self.nodes[0].assert_start_raises_init_error()

if __name__ == '__main__':
    AbortNodeTest().main()
