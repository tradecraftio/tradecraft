#!/usr/bin/env python3
# Copyright (c) 2019-2022 The Bitcoin Core developers
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
"""Useful Script constants and utils."""
from test_framework.script import (
    CScript,
    CScriptOp,
    OP_0,
    OP_1NEGATE,
    OP_1,
    OP_2,
    OP_3,
    OP_4,
    OP_5,
    OP_6,
    OP_7,
    OP_8,
    OP_9,
    OP_10,
    OP_11,
    OP_12,
    OP_13,
    OP_14,
    OP_15,
    OP_16,
    OP_NOP,
    OP_DEPTH,
    OP_CODESEPARATOR,
    OP_NOP1,
    OP_CHECKLOCKTIMEVERIFY,
    OP_CHECKSEQUENCEVERIFY,
    OP_MERKLEBRANCHVERIFY,
    OP_NOP5,
    OP_NOP6,
    OP_NOP7,
    OP_NOP8,
    OP_NOP9,
    OP_NOP10,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DUP,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_HASH160,
    OP_RETURN,
    hash160,
    hash256,
    ripemd160,
)

# To prevent a "tx-size-small" policy rule error, a transaction has to have a
# non-witness size of at least 65 bytes (MIN_STANDARD_TX_NONWITNESS_SIZE in
# src/policy/policy.h). Considering a Tx with the smallest possible single
# input (blank, empty scriptSig), and with an output omitting the scriptPubKey,
# we get to a minimum size of 60 bytes:
#
# Tx Skeleton: 4 [Version] + 1 [InCount] + 1 [OutCount] + 4 [LockTime] + 4 [LockHeight] = 14 bytes
# Blank Input: 32 [PrevTxHash] + 4 [Index] + 1 [scriptSigLen] + 4 [SeqNo] = 41 bytes
# Output:      8 [Amount] + 1 [scriptPubKeyLen] = 9 bytes
#
# Hence, the scriptPubKey of the single output has to have a size of at
# least 1 byte.
MIN_STANDARD_TX_NONWITNESS_SIZE = 65
MIN_PADDING = MIN_STANDARD_TX_NONWITNESS_SIZE - 14 - 41 - 9
assert MIN_PADDING == 1

# This script cannot be spent, allowing dust output values under
# standardness checks
DUMMY_MIN_OP_RETURN_SCRIPT = CScript([OP_RETURN] + ([OP_0] * (MIN_PADDING - 1)))
assert len(DUMMY_MIN_OP_RETURN_SCRIPT) == MIN_PADDING

def key_to_p2pk_script(key):
    key = check_key(key)
    return CScript([key, OP_CHECKSIG])


def keys_to_multisig_script(keys, *, k=None):
    n = len(keys)
    if k is None:  # n-of-n multisig by default
        k = n
    assert k <= n
    op_k = CScriptOp.encode_op_n(k)
    op_n = CScriptOp.encode_op_n(n)
    checked_keys = [check_key(key) for key in keys]
    return CScript([op_k] + checked_keys + [op_n, OP_CHECKMULTISIG])


def keyhash_to_p2pkh_script(hash):
    assert len(hash) == 20
    return CScript([OP_DUP, OP_HASH160, hash, OP_EQUALVERIFY, OP_CHECKSIG])


def scripthash_to_p2sh_script(hash):
    assert len(hash) == 20
    return CScript([OP_HASH160, hash, OP_EQUAL])


def key_to_p2pk_script(key):
    key = check_key(key)
    return CScript([key, OP_CHECKSIG])


def key_to_p2pkh_script(key):
    key = check_key(key)
    return keyhash_to_p2pkh_script(hash160(key))


def script_to_p2sh_script(script):
    script = check_script(script)
    return scripthash_to_p2sh_script(hash160(script))


def program_to_witness_script(version, program):
    if isinstance(program, str):
        program = bytes.fromhex(program)
    assert 0 <= version <= 30
    assert 2 <= len(program) <= 75
    version = [
        OP_0,
        OP_1NEGATE,
        OP_1,
        OP_2,
        OP_3,
        OP_4,
        OP_5,
        OP_6,
        OP_7,
        OP_8,
        OP_9,
        OP_10,
        OP_11,
        OP_12,
        OP_13,
        OP_14,
        OP_15,
        OP_16,
        OP_NOP,
        OP_DEPTH,
        OP_CODESEPARATOR,
        OP_NOP1,
        OP_CHECKLOCKTIMEVERIFY,
        OP_CHECKSEQUENCEVERIFY,
        OP_MERKLEBRANCHVERIFY,
        OP_NOP5,
        OP_NOP6,
        OP_NOP7,
        OP_NOP8,
        OP_NOP9,
        OP_NOP10,
    ][version]
    return CScript([version, program])


def script_to_witness(script):
    return b'\x00' + script


def script_to_p2wsh_script(script):
    script = check_script(script)
    return program_to_witness_script(0, hash256(script_to_witness(script)))


def script_to_p2wpk_script(script):
    script = check_script(script)
    return program_to_witness_script(0, ripemd160(hash256(script_to_witness(script))))


def key_to_p2wpk_script(key):
    key = check_key(key)
    return script_to_p2wpk_script(CScript([key, OP_CHECKSIG]))


def output_key_to_p2tr_script(key):
    assert len(key) == 32
    return program_to_witness_script(1, key)


def check_key(key):
    if isinstance(key, str):
        key = bytes.fromhex(key)  # Assuming this is hex string
    if isinstance(key, bytes) and (len(key) == 33 or len(key) == 65):
        return key
    assert False


def check_script(script):
    if isinstance(script, str):
        script = bytes.fromhex(script)  # Assuming this is hex string
    if isinstance(script, bytes) or isinstance(script, CScript):
        return script
    assert False
