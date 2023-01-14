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
"""Test block download

Ensure that even in IBD, we'll eventually sync chain from inbound peers
(whether we have only inbound peers or both inbound and outbound peers).
"""

from test_framework.test_framework import BitcoinTestFramework

class BlockSyncTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self):
        self.setup_nodes()
        # Construct a network:
        # node0 -> node1 -> node2
        # So node1 has both an inbound and outbound peer.
        # In our test, we will mine a block on node0, and ensure that it makes
        # to to both node1 and node2.
        self.connect_nodes(0, 1)
        self.connect_nodes(1, 2)

    def run_test(self):
        self.log.info("Setup network: node0->node1->node2")
        self.log.info("Mining one block on node0 and verify all nodes sync")
        self.generate(self.nodes[0], 1)
        self.log.info("Success!")


if __name__ == '__main__':
    BlockSyncTest().main()
