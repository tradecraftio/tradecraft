#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Copyright (c) 2010-2019 The Freicoin developers
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
# Test -reindex with CheckBlockIndex
#
from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *
import os.path

class ReindexTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 1)

    def setup_network(self):
        self.nodes = []
        self.is_network_split = False
        self.nodes.append(start_node(0, self.options.tmpdir))

    def run_test(self):
        self.nodes[0].generate(3)
        stop_node(self.nodes[0], 0)
        wait_bitcoinds()
        self.nodes[0]=start_node(0, self.options.tmpdir, ["-debug", "-reindex", "-checkblockindex=1"])
        assert_equal(self.nodes[0].getblockcount(), 3)
        print "Success"

if __name__ == '__main__':
    ReindexTest().main()
