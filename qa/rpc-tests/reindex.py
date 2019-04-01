#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2010-2019 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>

#
# Test -reindex and -reindex-chainstate with CheckBlockIndex
#
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    start_nodes,
    stop_nodes,
    assert_equal,
)
import time

class ReindexTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)

    def reindex(self, justchainstate=False):
        self.nodes[0].generate(3)
        blockcount = self.nodes[0].getblockcount()
        stop_nodes(self.nodes)
        extra_args = [["-debug", "-reindex-chainstate" if justchainstate else "-reindex", "-checkblockindex=1"]]
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, extra_args)
        while self.nodes[0].getblockcount() < blockcount:
            time.sleep(0.1)
        assert_equal(self.nodes[0].getblockcount(), blockcount)
        print("Success")

    def run_test(self):
        self.reindex(False)
        self.reindex(True)
        self.reindex(False)
        self.reindex(True)

if __name__ == '__main__':
    ReindexTest().main()
