#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Copyright (c) 2010-2022 The Freicoin Developers
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
"""A limited-functionality wallet, which may replace a real wallet in tests"""

from decimal import Decimal
from test_framework.address import ADDRESS_BCRT1_P2WSH_OP_TRUE
from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_TRUE,
)
from test_framework.util import (
    assert_equal,
    hex_str_to_bytes,
    kria_round,
)


class MiniWallet:
    def __init__(self, test_node):
        self._test_node = test_node
        self._utxos = []
        self._address = ADDRESS_BCRT1_P2WSH_OP_TRUE
        self._scriptPubKey = hex_str_to_bytes(self._test_node.validateaddress(self._address)['scriptPubKey'])

    def generate(self, num_blocks):
        """Generate blocks with coinbase outputs to the internal address, and append the outputs to the internal list"""
        blocks = self._test_node.generatetoaddress(num_blocks, self._address)
        for b in blocks:
            cb_tx = self._test_node.getblock(blockhash=b, verbosity=2)['tx'][0]
            self._utxos.append({'txid': cb_tx['txid'], 'vout': 0, 'value': cb_tx['vout'][0]['value'], 'refheight': cb_tx['lockheight']})
        return blocks

    def get_utxo(self):
        """Return the last utxo. Can be used to get the change output immediately after a send_self_transfer"""
        return self._utxos.pop()

    def send_self_transfer(self, *, fee_rate=Decimal("0.003"), from_node, utxo_to_spend=None):
        """Create and send a tx with the specified fee_rate. Fee may be exact or at most one kria higher than needed."""
        self._utxos = sorted(self._utxos, key=lambda k: k['value'])
        utxo_to_spend = utxo_to_spend or self._utxos.pop()  # Pick the largest utxo (if none provided) and hope it covers the fee
        vsize = Decimal(100)
        send_value = kria_round(utxo_to_spend['value'] - fee_rate * (vsize / 1000))
        fee = utxo_to_spend['value'] - send_value
        assert send_value > 0

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(utxo_to_spend['txid'], 16), utxo_to_spend['vout']))]
        tx.vout = [CTxOut(int(send_value * COIN), self._scriptPubKey)]
        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
        tx.lock_height = utxo_to_spend['refheight']
        tx_hex = tx.serialize().hex()

        txid = from_node.sendrawtransaction(tx_hex)
        self._utxos.append({'txid': txid, 'vout': 0, 'value': send_value, 'refheight': tx.lock_height})
        tx_info = from_node.getmempoolentry(txid)
        assert_equal(tx_info['vsize'], vsize)
        assert_equal(tx_info['fee'], fee)
        return {'txid': txid, 'wtxid': tx_info['wtxid'], 'hex': tx_hex}
