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
"""Useful util functions for testing the wallet"""
from collections import namedtuple

from test_framework.address import (
    key_to_p2pkh,
    key_to_p2wpk,
    script_to_p2sh,
    script_to_p2wsh,
)
from test_framework.script import (
    CScript,
    OP_0,
    OP_2,
    OP_3,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DUP,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_HASH160,
    hash160,
    hash256,
    ripemd160,
)
from test_framework.util import hex_str_to_bytes

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
    pkh = hash160(hex_str_to_bytes(pubkey))
    p2pk = CScript([hex_str_to_bytes(pubkey), OP_CHECKSIG])
    return Key(privkey=node.dumpprivkey(addr),
               pubkey=pubkey,
               p2pkh_script=CScript([OP_DUP, OP_HASH160, pkh, OP_EQUALVERIFY, OP_CHECKSIG]).hex(),
               p2pkh_addr=key_to_p2pkh(pubkey),
               p2wpk_script=CScript([OP_0, pkh]).hex(),
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
    script_code = CScript([OP_2] + [hex_str_to_bytes(pubkey) for pubkey in pubkeys] + [OP_3, OP_CHECKMULTISIG])
    witness_script = CScript([OP_0, hash256(b'\x00' + script_code)])
    return Multisig(privkeys=[node.dumpprivkey(addr) for addr in addrs],
                    pubkeys=pubkeys,
                    p2sh_script=CScript([OP_HASH160, hash160(script_code), OP_EQUAL]).hex(),
                    p2sh_addr=script_to_p2sh(script_code),
                    redeem_script=script_code.hex(),
                    p2wsh_script=witness_script.hex(),
                    p2wsh_addr=script_to_p2wsh(script_code),
                    witness_script="00" + script_code.hex())

def test_address(node, address, **kwargs):
    """Get address info for `address` and test whether the returned values are as expected."""
    addr_info = node.getaddressinfo(address)
    for key, value in kwargs.items():
        if value is None:
            if key in addr_info.keys():
                raise AssertionError("key {} unexpectedly returned in getaddressinfo.".format(key))
        elif addr_info[key] != value:
            raise AssertionError("key {} value {} did not match expected value {}".format(key, addr_info[key], value))
