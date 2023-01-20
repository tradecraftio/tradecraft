#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
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
"""Test UTXO set hash value calculation in gettxoutsetinfo."""

import struct

from test_framework.messages import (
    CBlock,
    COutPoint,
    from_hex,
)
from test_framework.muhash import MuHash3072
from test_framework.script import OP_RETURN
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

class UTXOSetHashTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_muhash_implementation(self):
        self.log.info("Test MuHash implementation consistency")

        node = self.nodes[0]
        wallet = MiniWallet(node)
        mocktime = node.getblockheader(node.getblockhash(0))['time'] + 1
        node.setmocktime(mocktime)

        # Generate 100 blocks
        block_hashes = self.generate(wallet, 1) + self.generate(node, 99)
        blocks = list(map(lambda block: from_hex(CBlock(), node.getblock(block, False)), block_hashes))

        # Create a spending transaction and mine a block which includes it
        txid = wallet.send_self_transfer(from_node=node)['txid']
        tx_block = self.generateblock(node, output=wallet.get_address(), transactions=[txid])
        blocks.append(from_hex(CBlock(), node.getblock(tx_block['hash'], False)))

        # Serialize the outputs that should be in the UTXO set and add them to
        # a MuHash object
        muhash = MuHash3072()

        for height, block in enumerate(blocks):
            # The Genesis block coinbase is not part of the UTXO set
            height += 1

            for tx in block.vtx:
                for n, tx_out in enumerate(tx.vout):
                    coinbase = 1 if not tx.vin[0].prevout.hash else 0

                    # Skip first two outputs of coinbase in first block,
                    # which were spent by us and by the first block-final
                    # transaction
                    if (height == 1 and coinbase and n <= 1):
                        continue

                    # Skip unspendable outputs
                    if (tx.vout[n].scriptPubKey[0] == OP_RETURN):
                        continue

                    data = COutPoint(int(tx.rehash(), 16), n).serialize()
                    data += struct.pack("<i", height * 2 + coinbase)
                    data += tx_out.serialize()
                    data += struct.pack("<I", tx.lock_height)

                    muhash.insert(data)

        finalized = muhash.digest()
        node_muhash = node.gettxoutsetinfo("muhash")['muhash']

        assert_equal(finalized[::-1].hex(), node_muhash)

        self.log.info("Test deterministic UTXO set hash results")
        assert_equal(node.gettxoutsetinfo()['hash_serialized_2'], "103a90290a0b3c473d7c32ff6069d7dd76b37db95035234ab4e0b9607abacef2")
        assert_equal(node.gettxoutsetinfo("muhash")['muhash'], "f4ab782b021bdcc438205c14d49c617db73e55e4f965d277958e16972ce8dd28")

    def run_test(self):
        self.test_muhash_implementation()


if __name__ == '__main__':
    UTXOSetHashTest().main()
