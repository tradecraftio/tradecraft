#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Freicoin developers
# Copyright (c) 2010-2021 The Freicoin Developers
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
# Test the BIP66 changeover logic
#

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import *

class BIP66Test(FreicoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 3
        self.setup_clean_chain = False

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, []))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-blockversion=2"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-blockversion=3"]))
        connect_nodes(self.nodes[1], 0)
        connect_nodes(self.nodes[2], 0)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        cnt = self.nodes[0].getblockcount()

        # Mine some old-version blocks
        self.nodes[1].generate(100)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 100):
            raise AssertionError("Failed to mine 100 version=2 blocks")

        # Mine 750 new-version blocks
        for i in range(15):
            self.nodes[2].generate(50)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 850):
            raise AssertionError("Failed to mine 750 version=3 blocks")

        # TODO: check that new DERSIG rules are not enforced

        # Mine 1 new-version block
        self.nodes[2].generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 851):
            raise AssertionError("Failed to mine a version=3 blocks")

        # TODO: check that new DERSIG rules are enforced

        # Mine 198 new-version blocks
        for i in range(2):
            self.nodes[2].generate(99)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 1049):
            raise AssertionError("Failed to mine 198 version=3 blocks")

        # Mine 1 old-version block
        self.nodes[1].generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 1050):
            raise AssertionError("Failed to mine a version=2 block after 949 version=3 blocks")

        # Mine 1 new-version blocks
        self.nodes[2].generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 1051):
            raise AssertionError("Failed to mine a version=3 block")

        # Mine 1 old-version blocks
        try:
            self.nodes[1].generate(1)
            raise AssertionError("Succeeded to mine a version=2 block after 950 version=3 blocks")
        except JSONRPCException:
            pass
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 1051):
            raise AssertionError("Accepted a version=2 block after 950 version=3 blocks")

        # Mine 1 new-version blocks
        self.nodes[2].generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 1052):
            raise AssertionError("Failed to mine a version=3 block")

if __name__ == '__main__':
    BIP66Test().main()
