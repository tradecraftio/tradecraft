#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
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
"""Test resurrection of mined transactions when the blockchain is re-organized."""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class MempoolCoinbaseTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        # Add enough mature utxos to the wallet so that all txs spend confirmed coins
        self.generate(wallet, 3)
        self.generate(node, COINBASE_MATURITY)

        # Spend block 1/2/3's coinbase transactions
        # Mine a block
        # Create three more transactions, spending the spends
        # Mine another block
        # ... make sure all the transactions are confirmed
        # Invalidate both blocks
        # ... make sure all the transactions are put back in the mempool
        # Mine a new block
        # ... make sure all the transactions are confirmed again
        blocks = []
        spends1_ids = [wallet.send_self_transfer(from_node=node)['txid'] for _ in range(3)]
        blocks.extend(self.generate(node, 1))
        spends2_ids = [wallet.send_self_transfer(from_node=node)['txid'] for _ in range(3)]
        blocks.extend(self.generate(node, 1))

        spends_ids = set(spends1_ids + spends2_ids)

        # mempool should be empty, all txns confirmed
        assert_equal(set(node.getrawmempool()), set())
        confirmed_txns = set(node.getblock(blocks[0])['tx'] + node.getblock(blocks[1])['tx'])
        # Checks that all spend txns are contained in the mined blocks
        assert spends_ids < confirmed_txns

        # Use invalidateblock to re-org back
        node.invalidateblock(blocks[0])

        # All txns should be back in mempool with 0 confirmations
        assert_equal(set(node.getrawmempool()), spends_ids)

        # Generate another block, they should all get mined
        blocks = self.generate(node, 1)
        # mempool should be empty, all txns confirmed
        assert_equal(set(node.getrawmempool()), set())
        confirmed_txns = set(node.getblock(blocks[0])['tx'])
        assert spends_ids < confirmed_txns


if __name__ == '__main__':
    MempoolCoinbaseTest().main()
