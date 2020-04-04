#!/usr/bin/env python3
# Copyright (c) 2016-2018 The Bitcoin Core developers
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
"""Encode and decode BASE58, P2PKH and P2SH addresses."""

from .script import hash256, hash160, ripemd160, CScript, CScriptOp, OP_0, OP_CHECKSIG
from .util import bytes_to_hex_str, hex_str_to_bytes

from . import segwit_addr

ADDRESS_BCRT1_UNSPENDABLE = 'bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq3xueyj'

chars = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'


def byte_to_base58(b, version):
    result = ''
    str = bytes_to_hex_str(b)
    str = bytes_to_hex_str(chr(version).encode('latin-1')) + str
    checksum = bytes_to_hex_str(hash256(hex_str_to_bytes(str)))
    str += checksum[:8]
    value = int('0x'+str,0)
    while value > 0:
        result = chars[value % 58] + result
        value //= 58
    while (str[:2] == '00'):
        result = chars[0] + result
        str = str[2:]
    return result

# TODO: def base58_decode

def keyhash_to_p2pkh(hash, main = False):
    assert (len(hash) == 20)
    version = 0 if main else 111
    return byte_to_base58(hash, version)

def scripthash_to_p2sh(hash, main = False):
    assert (len(hash) == 20)
    version = 5 if main else 196
    return byte_to_base58(hash, version)

def key_to_p2pkh(key, main = False):
    key = check_key(key)
    return keyhash_to_p2pkh(hash160(key), main)

def script_to_p2sh(script, main = False):
    script = check_script(script)
    return scripthash_to_p2sh(hash160(script), main)

def program_to_witness(version, program, main = False):
    if (type(program) is str):
        program = hex_str_to_bytes(program)
    assert 0 <= version <= 16
    assert 2 <= len(program) <= 40
    assert version > 0 or len(program) in [20, 32]
    return segwit_addr.encode("bc" if main else "bcrt", version, program)

def script_to_p2wsh(script, main = False):
    script = check_script(script)
    return program_to_witness(0, hash256(b'\x00' + script), main)

def key_to_p2wpk(key, main = False):
    key = check_key(key)
    return program_to_witness(0, ripemd160(hash256(b'\x00' + CScript([key, CScriptOp(OP_CHECKSIG)]))), main)

def check_key(key):
    if (type(key) is str):
        key = hex_str_to_bytes(key) # Assuming this is hex string
    if (type(key) is bytes and (len(key) == 33 or len(key) == 65)):
        return key
    assert(False)

def check_script(script):
    if (type(script) is str):
        script = hex_str_to_bytes(script) # Assuming this is hex string
    if (type(script) is bytes or type(script) is CScript):
        return script
    assert(False)
