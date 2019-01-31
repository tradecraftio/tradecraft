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
"""Test datacarrier functionality"""
from test_framework.messages import (
    CTxOut,
    MAX_OP_RETURN_RELAY,
)
from test_framework.script import (
    CScript,
    OP_RETURN,
)
from test_framework.test_framework import FreicoinTestFramework
from test_framework.test_node import TestNode
from test_framework.util import (
    assert_raises_rpc_error,
    random_bytes,
)
from test_framework.wallet import MiniWallet


class DataCarrierTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            [],
            ["-datacarrier=0"],
            ["-datacarrier=1", f"-datacarriersize={MAX_OP_RETURN_RELAY - 1}"]
        ]

    def test_null_data_transaction(self, node: TestNode, data: bytes, success: bool) -> None:
        tx = self.wallet.create_self_transfer(fee_rate=0)["tx"]
        tx.vout.append(CTxOut(nValue=0, scriptPubKey=CScript([OP_RETURN, data])))
        tx.vout[0].nValue -= tx.get_vsize()  # simply pay 1sat/vbyte fee

        tx_hex = tx.serialize().hex()

        if success:
            self.wallet.sendrawtransaction(from_node=node, tx_hex=tx_hex)
            assert tx.rehash() in node.getrawmempool(True), f'{tx_hex} not in mempool'
        else:
            assert_raises_rpc_error(-26, "scriptpubkey", self.wallet.sendrawtransaction, from_node=node, tx_hex=tx_hex)

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.wallet.rescan_utxos()

        # By default, only 80 bytes are used for data (+1 for OP_RETURN, +2 for the pushdata opcodes).
        default_size_data = random_bytes(MAX_OP_RETURN_RELAY - 3)
        too_long_data = random_bytes(MAX_OP_RETURN_RELAY - 2)
        small_data = random_bytes(MAX_OP_RETURN_RELAY - 4)

        self.log.info("Testing null data transaction with default -datacarrier and -datacarriersize values.")
        self.test_null_data_transaction(node=self.nodes[0], data=default_size_data, success=False)

        self.log.info("Testing a null data transaction larger than allowed by the default -datacarriersize value.")
        self.test_null_data_transaction(node=self.nodes[0], data=too_long_data, success=False)

        self.log.info("Testing a null data transaction with -datacarrier=false.")
        self.test_null_data_transaction(node=self.nodes[1], data=default_size_data, success=False)

        self.log.info("Testing a null data transaction with a size larger than accepted by -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], data=default_size_data, success=False)

        self.log.info("Testing a null data transaction with a size smaller than accepted by -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], data=small_data, success=True)


if __name__ == '__main__':
    DataCarrierTest().main()
