#!/usr/bin/env python3
# Copyright (c) 2018-2021 The Bitcoin Core developers
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
"""Useful util functions for testing the wallet"""
from collections import namedtuple

from test_framework.address import (
    byte_to_base58,
    key_to_p2pkh,
    key_to_p2wpk,
    script_to_p2sh,
    script_to_p2wsh,
    script_to_witscript,
)
from test_framework.key import ECKey
from test_framework.script_util import (
    key_to_p2pkh_script,
    key_to_p2wpk_script,
    keys_to_multisig_script,
    script_to_p2sh_script,
    script_to_p2wsh_script,
)

Key = namedtuple('Key', ['privkey',
                         'pubkey',
                         'p2pkh_script',
                         'p2pkh_addr',
                         'p2wpk_script',
                         'p2wpk_addr'])

Multisig = namedtuple('Multisig', ['privkeys',
                                   'pubkeys',
                                   'p2sh_script',
                                   'p2sh_addr',
                                   'redeem_script',
                                   'p2wsh_script',
                                   'p2wsh_addr',
                                   'witness_script'])

def get_key(node):
    """Generate a fresh key on node

    Returns a named tuple of privkey, pubkey and all address and scripts."""
    addr = node.getnewaddress()
    pubkey = node.getaddressinfo(addr)['pubkey']
    return Key(privkey=node.dumpprivkey(addr),
               pubkey=pubkey,
               p2pkh_script=key_to_p2pkh_script(pubkey).hex(),
               p2pkh_addr=key_to_p2pkh(pubkey),
               p2wpk_script=key_to_p2wpk_script(pubkey).hex(),
               p2wpk_addr=key_to_p2wpk(pubkey))

def get_generate_key():
    """Generate a fresh key

    Returns a named tuple of privkey, pubkey and all address and scripts."""
    privkey, pubkey = generate_keypair(wif=True)
    return Key(privkey=privkey,
               pubkey=pubkey.hex(),
               p2pkh_script=key_to_p2pkh_script(pubkey).hex(),
               p2pkh_addr=key_to_p2pkh(pubkey),
               p2wpk_script=key_to_p2wpk_script(pubkey).hex(),
               p2wpk_addr=key_to_p2wpk(pubkey))

def get_multisig(node):
    """Generate a fresh 2-of-3 multisig on node

    Returns a named tuple of privkeys, pubkeys and all address and scripts."""
    addrs = []
    pubkeys = []
    for _ in range(3):
        addr = node.getaddressinfo(node.getnewaddress())
        addrs.append(addr['address'])
        pubkeys.append(addr['pubkey'])
    script_code = keys_to_multisig_script(pubkeys, k=2)
    witness_script = script_to_p2wsh_script(script_code)
    return Multisig(privkeys=[node.dumpprivkey(addr) for addr in addrs],
                    pubkeys=pubkeys,
                    p2sh_script=script_to_p2sh_script(script_code).hex(),
                    p2sh_addr=script_to_p2sh(script_code),
                    redeem_script=script_code.hex(),
                    p2wsh_script=witness_script.hex(),
                    p2wsh_addr=script_to_p2wsh(script_code),
                    witness_script=script_to_witscript(script_code).hex())

def test_address(node, address, **kwargs):
    """Get address info for `address` and test whether the returned values are as expected."""
    addr_info = node.getaddressinfo(address)
    for key, value in kwargs.items():
        if value is None:
            if key in addr_info.keys():
                raise AssertionError("key {} unexpectedly returned in getaddressinfo.".format(key))
        elif addr_info[key] != value:
            raise AssertionError("key {} value {} did not match expected value {}".format(key, addr_info[key], value))

def bytes_to_wif(b, compressed=True):
    if compressed:
        b += b'\x01'
    return byte_to_base58(b, 239)

def generate_keypair(compressed=True, wif=False):
    """Generate a new random keypair and return the corresponding ECKey /
    bytes objects. The private key can also be provided as WIF (wallet
    import format) string instead, which is often useful for wallet RPC
    interaction."""
    privkey = ECKey()
    privkey.generate(compressed)
    pubkey = privkey.get_pubkey().get_bytes()
    if wif:
        privkey = bytes_to_wif(privkey.get_bytes(), compressed)
    return privkey, pubkey

class WalletUnlock():
    """
    A context manager for unlocking a wallet with a passphrase and automatically locking it afterward.
    """

    MAXIMUM_TIMEOUT = 999000

    def __init__(self, wallet, passphrase, timeout=MAXIMUM_TIMEOUT):
        self.wallet = wallet
        self.passphrase = passphrase
        self.timeout = timeout

    def __enter__(self):
        self.wallet.walletpassphrase(self.passphrase, self.timeout)

    def __exit__(self, *args):
        _ = args
        self.wallet.walletlock()
