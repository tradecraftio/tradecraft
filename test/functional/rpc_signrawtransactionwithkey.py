#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Copyright (c) 2010-2024 The Freicoin Developers
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
"""Test transaction signing using the signrawtransactionwithkey RPC."""

from test_framework.blocktools import (
    COINBASE_MATURITY,
)
from test_framework.address import (
    address_to_scriptpubkey,
    script_to_p2sh,
    script_to_p2wsh,
)
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    find_vout_for_address,
)
from test_framework.script_util import (
    key_to_p2pk_script,
    key_to_p2pkh_script,
    script_to_p2wsh_script,
)
from test_framework.wallet import (
    getnewdestination,
)
from test_framework.wallet_util import (
    generate_keypair,
)

from decimal import (
    Decimal,
)

INPUTS = [
    # Valid pay-to-pubkey scripts
    {'txid': '9b907ef1e3c26fc71fe4a4b3580bc75264112f95050014157059c736f0202e71', 'vout': 0,
     'scriptPubKey': '76a91460baa0f494b38ce3c940dea67f3804dc52d1fb9488ac', 'refheight': 1},
    {'txid': '83a4f6a6b73660e13ee6cb3c6063fa3759c50c9b7521d0536022961898f4fb02', 'vout': 0,
     'scriptPubKey': '76a914669b857c03a5ed269d5d85a1ffac9ed5d663072788ac', 'refheight': 1},
]
OUTPUTS = {'mpLQjfK79b7CCV4VMJWEWAj5Mpx8Up5zxB': 0.1}

class SignRawTransactionWithKeyTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def send_to_address(self, addr, amount):
        input = {"txid": self.nodes[0].getblock(self.block_hash[self.blk_idx])["tx"][0], "vout": 0}
        output = {addr: amount}
        self.blk_idx += 1
        rawtx = self.nodes[0].createrawtransaction([input], output)
        txid = self.nodes[0].sendrawtransaction(self.nodes[0].signrawtransactionwithkey(rawtx, [self.nodes[0].get_deterministic_priv_key().key])["hex"], 0)
        return txid

    def assert_signing_completed_successfully(self, signed_tx):
        assert 'errors' not in signed_tx
        assert 'complete' in signed_tx
        assert_equal(signed_tx['complete'], True)

    def successful_signing_test(self):
        """Create and sign a valid raw transaction with one input.

        Expected results:

        1) The transaction has a complete set of signatures
        2) No script verification error occurred"""
        self.log.info("Test valid raw transaction with one input")
        privKeys = ['cUeKHd5orzT3mz8P9pxyREHfsWtVfgsfDjiZZBcjUBAaGk1BTj7N', 'cVKpPfVKSJxKqVpE9awvXNWuLHCa5j5tiE7K6zbUSptFpTEtiFrA']
        rawTx = self.nodes[0].createrawtransaction(INPUTS, OUTPUTS)
        rawTxSigned = self.nodes[0].signrawtransactionwithkey(rawTx, privKeys, INPUTS)

        self.assert_signing_completed_successfully(rawTxSigned)

    def witness_script_test(self):
        self.log.info("Test signing transaction to P2SH-P2WSH addresses without wallet")
        # Create a new P2SH-P2WSH 1-of-1 multisig address:
        embedded_privkey, embedded_pubkey = generate_keypair(wif=True)
        p2wsh_address = self.nodes[1].createmultisig(1, [embedded_pubkey.hex()], "bech32")
        # send transaction to P2SH-P2WSH 1-of-1 multisig address
        self.block_hash = self.generate(self.nodes[0], COINBASE_MATURITY + 2)
        self.blk_idx = 1
        self.send_to_address(p2wsh_address["address"], 49.999)
        self.generate(self.nodes[0], 1)
        # Get the UTXO info from scantxoutset
        unspent_output = self.nodes[1].scantxoutset('start', [p2wsh_address['descriptor']])['unspents'][0]
        spk = script_to_p2wsh_script(p2wsh_address['redeemScript']).hex()
        unspent_output['witnessScript'] = '00' + p2wsh_address['redeemScript']
        unspent_output['redeemScript'] = script_to_p2wsh_script(p2wsh_address['redeemScript']).hex()
        assert_equal(spk, unspent_output['scriptPubKey'])
        # Now create and sign a transaction spending that output on node[0], which doesn't know the scripts or keys
        spending_tx = self.nodes[0].createrawtransaction([unspent_output], {getnewdestination()[2]: Decimal("49.998")})
        spending_tx_signed = self.nodes[0].signrawtransactionwithkey(spending_tx, [embedded_privkey], [unspent_output])
        self.assert_signing_completed_successfully(spending_tx_signed)

        # Now test with P2PKH and P2PK scripts as the witnessScript
        for tx_type in ['P2PKH', 'P2PK']:  # these tests are order-independent
            self.verify_txn_with_witness_script(tx_type)

    def verify_txn_with_witness_script(self, tx_type):
        self.log.info("Test with a {} script as the witnessScript".format(tx_type))
        embedded_privkey, embedded_pubkey = generate_keypair(wif=True)
        witness_script = '00' + {
            'P2PKH': key_to_p2pkh_script(embedded_pubkey).hex(),
            'P2PK': key_to_p2pk_script(embedded_pubkey).hex()
        }.get(tx_type, "Invalid tx_type")
        redeem_script = script_to_p2wsh_script(witness_script[2:]).hex()
        addr = self.nodes[1].decodescript(redeem_script)['address']
        script_pub_key = address_to_scriptpubkey(addr).hex()
        # Fund that address
        txid = self.send_to_address(addr, 10)
        vout = find_vout_for_address(self.nodes[0], txid, addr)
        self.generate(self.nodes[0], 1)
        # Now create and sign a transaction spending that output on node[0], which doesn't know the scripts or keys
        spending_tx = self.nodes[0].createrawtransaction([{'txid': txid, 'vout': vout}], {getnewdestination()[2]: Decimal("9.999")})
        spending_tx_signed = self.nodes[0].signrawtransactionwithkey(spending_tx, [embedded_privkey], [{'txid': txid, 'vout': vout, 'refheight': self.nodes[0].gettxout(txid, vout)['refheight'], 'scriptPubKey': script_pub_key, 'redeemScript': redeem_script, 'witnessScript': witness_script, 'value': 10}])
        self.assert_signing_completed_successfully(spending_tx_signed)
        self.nodes[0].sendrawtransaction(spending_tx_signed['hex'])

    def invalid_sighashtype_test(self):
        self.log.info("Test signing transaction with invalid sighashtype")
        tx = self.nodes[0].createrawtransaction(INPUTS, OUTPUTS)
        privkeys = [self.nodes[0].get_deterministic_priv_key().key]
        assert_raises_rpc_error(-8, "'all' is not a valid sighash parameter.", self.nodes[0].signrawtransactionwithkey, tx, privkeys, sighashtype="all")

    def run_test(self):
        self.successful_signing_test()
        self.witness_script_test()
        self.invalid_sighashtype_test()


if __name__ == '__main__':
    SignRawTransactionWithKeyTest().main()
