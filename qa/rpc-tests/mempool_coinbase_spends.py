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
# Test re-org scenarios with a mempool that contains transactions
# that spend (directly or indirectly) coinbase transactions.
#

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *
import os
import shutil

# Create one-input, one-output, no-fee transaction:
class MempoolCoinbaseTest(BitcoinTestFramework):

    alert_filename = None  # Set by setup_network

    def setup_network(self):
        args = ["-checkmempool", "-debug=mempool"]
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, args))
        self.nodes.append(start_node(1, self.options.tmpdir, args))
        connect_nodes(self.nodes[1], 0)
        self.is_network_split = False
        self.sync_all

    def create_tx(self, from_txid, to_address, amount):
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        signresult = self.nodes[0].signrawtransaction(rawtx)
        assert_equal(signresult["complete"], True)
        return signresult["hex"]

    def run_test(self):
        start_count = self.nodes[0].getblockcount()

        # Mine three blocks. After this, nodes[0] blocks
        # 101, 102, and 103 are spend-able.
        new_blocks = self.nodes[1].setgenerate(True, 4)
        self.sync_all()

        node0_address = self.nodes[0].getnewaddress()
        node1_address = self.nodes[1].getnewaddress()

        # Three scenarios for re-orging coinbase spends in the memory pool:
        # 1. Direct coinbase spend  :  spend_101
        # 2. Indirect (coinbase spend in chain, child in mempool) : spend_102 and spend_102_1
        # 3. Indirect (coinbase and child both in chain) : spend_103 and spend_103_1
        # Use invalidatblock to make all of the above coinbase spends invalid (immature coinbase),
        # and make sure the mempool code behaves correctly.
        b = [ self.nodes[0].getblockhash(n) for n in range(102, 105) ]
        coinbase_txids = [ self.nodes[0].getblock(h)['tx'][0] for h in b ]
        spend_101_raw = self.create_tx(coinbase_txids[0], node1_address, 50)
        spend_102_raw = self.create_tx(coinbase_txids[1], node0_address, 50)
        spend_103_raw = self.create_tx(coinbase_txids[2], node0_address, 50)

        # Broadcast and mine spend_102 and 103:
        spend_102_id = self.nodes[0].sendrawtransaction(spend_102_raw)
        spend_103_id = self.nodes[0].sendrawtransaction(spend_103_raw)
        self.nodes[0].setgenerate(True, 1)

        # Create 102_1 and 103_1:
        spend_102_1_raw = self.create_tx(spend_102_id, node1_address, 50)
        spend_103_1_raw = self.create_tx(spend_103_id, node1_address, 50)

        # Broadcast and mine 103_1:
        spend_103_1_id = self.nodes[0].sendrawtransaction(spend_103_1_raw)
        self.nodes[0].setgenerate(True, 1)

        # ... now put spend_101 and spend_102_1 in memory pools:
        spend_101_id = self.nodes[0].sendrawtransaction(spend_101_raw)
        spend_102_1_id = self.nodes[0].sendrawtransaction(spend_102_1_raw)

        self.sync_all()

        assert_equal(set(self.nodes[0].getrawmempool()), set([ spend_101_id, spend_102_1_id ]))

        # Use invalidateblock to re-org back and make all those coinbase spends
        # immature/invalid:
        for node in self.nodes:
            node.invalidateblock(new_blocks[0])

        self.sync_all()

        # mempool should be empty.
        assert_equal(set(self.nodes[0].getrawmempool()), set())

if __name__ == '__main__':
    MempoolCoinbaseTest().main()
