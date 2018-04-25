#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
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
# Test resurrection of mined transactions when
# the blockchain is re-organized.
#

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import *
import os
import shutil

# Create one-input, one-output, no-fee transaction:
class MempoolCoinbaseTest(FreicoinTestFramework):

    def setup_network(self):
        # Just need one node for this test
        args = ["-checkmempool", "-debug=mempool"]
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, args))
        self.is_network_split = False

    def create_tx(self, from_txid, to_address, amount):
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        signresult = self.nodes[0].signrawtransaction(rawtx)
        assert_equal(signresult["complete"], True)
        return signresult["hex"]

    def run_test(self):
        node0_address = self.nodes[0].getnewaddress()
        # Spend block 1/2/3's coinbase transactions
        # Mine a block.
        # Create three more transactions, spending the spends
        # Mine another block.
        # ... make sure all the transactions are confirmed
        # Invalidate both blocks
        # ... make sure all the transactions are put back in the mempool
        # Mine a new block
        # ... make sure all the transactions are confirmed again.

        b = [ self.nodes[0].getblockhash(n) for n in range(1, 4) ]
        coinbase_txids = [ self.nodes[0].getblock(h)['tx'][0] for h in b ]
        spends1_raw = [ self.create_tx(txid, node0_address, 50) for txid in coinbase_txids ]
        spends1_id = [ self.nodes[0].sendrawtransaction(tx) for tx in spends1_raw ]

        blocks = []
        blocks.extend(self.nodes[0].generate(1))

        spends2_raw = [ self.create_tx(txid, node0_address, 49.99) for txid in spends1_id ]
        spends2_id = [ self.nodes[0].sendrawtransaction(tx) for tx in spends2_raw ]

        blocks.extend(self.nodes[0].generate(1))

        # mempool should be empty, all txns confirmed
        assert_equal(set(self.nodes[0].getrawmempool()), set())
        for txid in spends1_id+spends2_id:
            tx = self.nodes[0].gettransaction(txid)
            assert(tx["confirmations"] > 0)

        # Use invalidateblock to re-org back; all transactions should
        # end up unconfirmed and back in the mempool
        for node in self.nodes:
            node.invalidateblock(blocks[0])

        # Use an address from the keypool so that next block generated
        # will have a different coinbase transaction and therefore not
        # be literally identical to the invalidated block, which would
        # cause problems.
        self.nodes[0].getnewaddress()

        # mempool should be empty, all txns confirmed
        assert_equal(set(self.nodes[0].getrawmempool()), set(spends1_id+spends2_id))
        for txid in spends1_id+spends2_id:
            tx = self.nodes[0].gettransaction(txid)
            assert(tx["confirmations"] == 0)

        # Generate another block, spends1 should get mined. spends2
        # has a non-final lock_height and so it remains in the
        # mempool.
        self.nodes[0].generate(1)
        # mempool should contain just spends2, spends1 is confirmed
        assert_equal(set(self.nodes[0].getrawmempool()), set(spends2_id))
        for txid in spends1_id:
            tx = self.nodes[0].gettransaction(txid)
            assert(tx["confirmations"] > 0)
        for txid in spends2_id:
            tx = self.nodes[0].gettransaction(txid)
            assert(tx["confirmations"] == 0)

        # Generate another block, they should all get mined
        self.nodes[0].generate(1)
        # mempool should be empty, all txns confirmed
        assert_equal(set(self.nodes[0].getrawmempool()), set())
        for txid in spends1_id+spends2_id:
            tx = self.nodes[0].gettransaction(txid)
            assert(tx["confirmations"] > 0)


if __name__ == '__main__':
    MempoolCoinbaseTest().main()
