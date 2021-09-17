#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
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
"""Test node responses to invalid blocks.

In this test we connect to one node over p2p, and test block requests:
1) Valid blocks should be requested and become chain tip.
2) Invalid block with duplicated transaction should be re-requested.
3) Invalid block with bad coinbase value should be rejected and not
re-requested.
"""
import copy

from test_framework.blocktools import create_block, create_coinbase, create_tx_with_script, get_final_tx_info, add_final_tx
from test_framework.messages import COIN
from test_framework.mininode import P2PDataStore
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal

class InvalidBlockRequestTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-whitelist=127.0.0.1"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Add p2p connection to node0
        node = self.nodes[0]  # convenience reference to the node
        node.add_p2p_connection(P2PDataStore())

        # Let freicoind handle the block-final initial output logic
        self.nodes[0].generate(1)

        best_block = node.getblock(node.getbestblockhash())
        tip = int(node.getbestblockhash(), 16)
        height = best_block["height"] + 1
        block_time = best_block["time"] + 1

        self.log.info("Create a new block with an anyone-can-spend coinbase")

        height = self.nodes[0].getblockcount() + 1
        block = create_block(tip, create_coinbase(height), block_time)
        block.solve()
        # Save the coinbase for later
        block1 = block
        tip = block.sha256
        node.p2p.send_blocks_and_test([block1], node, success=True)

        self.log.info("Mature the block.")
        node.generate(100)

        best_block = node.getblock(node.getbestblockhash())
        tip = int(node.getbestblockhash(), 16)
        height = best_block["height"] + 1
        block_time = best_block["time"] + 1
        final_tx = get_final_tx_info(node)

        # Use merkle-root malleability to generate an invalid block with
        # same blockheader.
        # Manufacture a block with 3 transactions (coinbase, spend of prior
        # coinbase, spend of that spend).  Duplicate the 3rd transaction to
        # leave merkle root and blockheader unchanged but invalidate the block.
        self.log.info("Test merkle root malleability.")

        block2 = create_block(tip, create_coinbase(height), block_time)
        block_time += 1

        # b'0x51' is OP_TRUE
        tx1 = create_tx_with_script(block1.vtx[0], 0, script_sig=b'\x51', amount=50 * COIN)
        tx2 = create_tx_with_script(tx1, 0, script_sig=b'\x51', amount=50 * COIN)

        block2.vtx.extend([tx1])
        block2.hashMerkleRoot = block2.calc_merkle_root()
        add_final_tx(final_tx, block2)
        block2.rehash()
        block2.solve()
        orig_hash = block2.sha256
        block2_orig = copy.deepcopy(block2)

        # Mutate block 2
        block2.vtx.append(block2.vtx[-1])
        assert_equal(block2.hashMerkleRoot, block2.calc_merkle_root())
        assert_equal(orig_hash, block2.rehash())
        assert block2_orig.vtx != block2.vtx

        node.p2p.send_blocks_and_test([block2], node, success=False, reject_code=16, reject_reason=b'bad-txns-duplicate')

        # Check transactions for duplicate inputs
        self.log.info("Test duplicate input block.")

        block2_orig.vtx[1].vin.append(block2_orig.vtx[1].vin[0])
        block2_orig.vtx[1].rehash()
        block2_orig.hashMerkleRoot = block2_orig.calc_merkle_root()
        block2_orig.rehash()
        block2_orig.solve()
        node.p2p.send_blocks_and_test([block2_orig], node, success=False, reject_reason=b'bad-txns-inputs-duplicate')

        self.log.info("Test very broken block.")

        block3 = create_block(tip, create_coinbase(height), block_time)
        block_time += 1
        block3.vtx[0].vout[0].nValue = 100 * COIN  # Too high!
        block3.vtx[0].sha256 = None
        block3.vtx[0].calc_sha256()
        block3.hashMerkleRoot = block3.calc_merkle_root()
        final_tx = add_final_tx(final_tx, block3)
        block3.rehash()
        block3.solve()

        node.p2p.send_blocks_and_test([block3], node, success=False, reject_code=16, reject_reason=b'bad-cb-amount')

if __name__ == '__main__':
    InvalidBlockRequestTest().main()
