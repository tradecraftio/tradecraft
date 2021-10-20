#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
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
"""Test createwallet arguments.
"""

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class CreateWalletTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.supports_cli = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        node.generate(1) # Leave IBD for sethdseed

        self.nodes[0].createwallet(wallet_name='w0')
        w0 = node.get_wallet_rpc('w0')
        address1 = w0.getnewaddress()

        self.log.info("Test disableprivatekeys creation.")
        self.nodes[0].createwallet(wallet_name='w1', disable_private_keys=True)
        w1 = node.get_wallet_rpc('w1')
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w1.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w1.getrawchangeaddress)
        w1.importpubkey(w0.getaddressinfo(address1)['pubkey'])

        self.log.info('Test that private keys cannot be imported')
        addr = w0.getnewaddress('', 'legacy')
        privkey = w0.dumpprivkey(addr)
        assert_raises_rpc_error(-4, 'Cannot import private keys to a wallet with private keys disabled', w1.importprivkey, privkey)
        result = w1.importmulti([{'scriptPubKey': {'address': addr}, 'timestamp': 'now', 'keys': [privkey]}])
        assert(not result[0]['success'])
        assert('warning' not in result[0])
        assert_equal(result[0]['error']['code'], -4)
        assert_equal(result[0]['error']['message'], 'Cannot import private keys to a wallet with private keys disabled')

        self.log.info("Test blank creation with private keys disabled.")
        self.nodes[0].createwallet(wallet_name='w2', disable_private_keys=True, blank=True)
        w2 = node.get_wallet_rpc('w2')
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w2.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w2.getrawchangeaddress)
        w2.importpubkey(w0.getaddressinfo(address1)['pubkey'])

        self.log.info("Test blank creation with private keys enabled.")
        self.nodes[0].createwallet(wallet_name='w3', disable_private_keys=False, blank=True)
        w3 = node.get_wallet_rpc('w3')
        assert_equal(w3.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w3.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w3.getrawchangeaddress)
        # Import private key
        w3.importprivkey(w0.dumpprivkey(address1))
        # Imported private keys are currently ignored by the keypool
        assert_equal(w3.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w3.getnewaddress)
        # Set the seed
        w3.sethdseed()
        assert_equal(w3.getwalletinfo()['keypoolsize'], 1)
        w3.getnewaddress()
        w3.getrawchangeaddress()

        self.log.info("Test blank creation with privkeys enabled and then encryption")
        self.nodes[0].createwallet(wallet_name='w4', disable_private_keys=False, blank=True)
        w4 = node.get_wallet_rpc('w4')
        assert_equal(w4.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w4.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w4.getrawchangeaddress)
        # Encrypt the wallet. Nothing should change about the keypool
        w4.encryptwallet('pass')
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w4.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w4.getrawchangeaddress)
        # Now set a seed and it should work. Wallet should also be encrypted
        w4.walletpassphrase('pass', 2)
        w4.sethdseed()
        w4.getnewaddress()
        w4.getrawchangeaddress()

        self.log.info("Test blank creation with privkeys disabled and then encryption")
        self.nodes[0].createwallet(wallet_name='w5', disable_private_keys=True, blank=True)
        w5 = node.get_wallet_rpc('w5')
        assert_equal(w5.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w5.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w5.getrawchangeaddress)
        # Encrypt the wallet
        w5.encryptwallet('pass')
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w5.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w5.getrawchangeaddress)

if __name__ == '__main__':
    CreateWalletTest().main()
