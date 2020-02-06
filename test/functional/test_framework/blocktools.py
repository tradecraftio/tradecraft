#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Core developers
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
"""Utilities for manipulating blocks and transactions."""

from .address import (
    key_to_p2sh_p2wpk,
    key_to_p2wpk,
    script_to_p2sh_p2wsh,
    script_to_p2wsh,
)
from .mininode import *
from .script import (
    CScript,
    OP_0,
    OP_1,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_TRUE,
    hash160,
)
from .util import (
    assert_equal,
    unhexlify,
)

# Create a block (with regtest difficulty)
def create_block(hashprev, coinbase, nTime=None):
    block = CBlock()
    if nTime is None:
        import time
        block.nTime = int(time.time()+600)
    else:
        block.nTime = nTime
    block.hashPrevBlock = hashprev
    block.nBits = 0x207fffff # Will break after a difficulty adjustment...
    block.vtx.append(coinbase)
    block.hashMerkleRoot = block.calc_merkle_root()
    block.calc_sha256()
    return block

def get_final_tx_info(node):
    try:
        finaltx_prevout = node.getblocktemplate({'rules':['finaltx','segwit']})['finaltx']['prevout']
    except KeyError:
        finaltx_prevout = []
    return finaltx_prevout

def add_final_tx(info, block):
    finaltx = CTransaction()
    finaltx.nLockTime = block.vtx[0].nLockTime
    finaltx.lock_height = block.vtx[0].lock_height
    finaltx.vout.append(CTxOut(0, CScript([OP_TRUE])))
    for prevout in info:
        finaltx.vin.append(CTxIn(COutPoint(uint256_from_str(unhexlify(prevout['txid'])[::-1]), prevout['vout']), CScript([]), 0xffffffff))
        finaltx.vout[-1].nValue += prevout['amount']
    finaltx.rehash()
    block.vtx.append(finaltx)
    block.hashMerkleRoot = block.calc_merkle_root()
    return [{
        'txid': finaltx.hash,
        'vout': 0,
        'amount': int(finaltx.vout[-1].nValue),
    }]

# From BIP141
WITNESS_COMMITMENT_HEADER = b"\x4b\x4a\x49\x48"


def get_witness_script(witness_root, witness_nonce):
    witness_path = 0x01
    if witness_nonce:
        witness_path = 0x02
        witness_root = uint256_from_str(fastHash256(ser_uint256(witness_root),
                                                    ser_uint256(witness_nonce)))
    output_data = bytes((witness_path,)) + ser_uint256(witness_root) + WITNESS_COMMITMENT_HEADER
    return CScript([output_data])


# According to BIP141, blocks with witness rules active must commit to the
# hash of all in-block transactions including witness.
def add_witness_commitment(block, nonce=0):
    # First calculate the merkle root of the block's
    # transactions, with witnesses.
    witness_branch = b''
    if nonce:
        witness_branch = ser_uint256(nonce)
    output_data = bytes((0,)) + ser_uint256(0) + WITNESS_COMMITMENT_HEADER
    block.vtx[-1].vout[-1].scriptPubKey = CScript([output_data])
    block.vtx[-1].rehash()
    witness_root = block.calc_witness_merkle_root()
    # witness_branch should go to coinbase witness.
    block.vtx[0].wit.vtxinwit = [CTxInWitness()]
    block.vtx[0].wit.vtxinwit[0].scriptWitness.stack = [witness_branch]

    # witness commitment is the last output in coinbase
    block.vtx[-1].vout[-1].scriptPubKey = get_witness_script(witness_root, nonce)
    block.vtx[-1].rehash()
    block.hashMerkleRoot = block.calc_merkle_root()
    block.rehash()


def serialize_script_num(value):
    r = bytearray(0)
    if value == 0:
        return r
    neg = value < 0
    absvalue = -value if neg else value
    while (absvalue):
        r.append(int(absvalue & 0xff))
        absvalue >>= 8
    if r[-1] & 0x80:
        r.append(0x80 if neg else 0)
    elif neg:
        r[-1] |= 0x80
    return r

# Create a coinbase transaction, assuming no miner fees.
# If pubkey is passed in, the coinbase output will be a P2PK output;
# otherwise an anyone-can-spend output.
def create_coinbase(height, pubkey = None):
    coinbase = CTransaction()
    coinbase.vin.append(CTxIn(COutPoint(0, 0xffffffff), 
                ser_string(serialize_script_num(height)), 0xffffffff))
    coinbaseoutput = CTxOut()
    coinbaseoutput.nValue = 50 * COIN
    halvings = int(height/150) # regtest
    coinbaseoutput.nValue >>= halvings
    if (pubkey != None):
        coinbaseoutput.scriptPubKey = CScript([pubkey, OP_CHECKSIG])
    else:
        coinbaseoutput.scriptPubKey = CScript([OP_TRUE])
    coinbase.vout = [ coinbaseoutput ]
    coinbase.lock_height = height
    coinbase.calc_sha256()
    return coinbase

# Create a transaction.
# If the scriptPubKey is not specified, make it anyone-can-spend.
def create_transaction(prevtx, n, sig, value, scriptPubKey=CScript()):
    tx = CTransaction()
    tx.lock_height = prevtx.lock_height
    assert(n < len(prevtx.vout))
    tx.vin.append(CTxIn(COutPoint(prevtx.sha256, n), sig, 0xffffffff))
    tx.vout.append(CTxOut(value, scriptPubKey))
    tx.calc_sha256()
    return tx

def get_legacy_sigopcount_block(block, fAccurate=True):
    count = 0
    for tx in block.vtx:
        count += get_legacy_sigopcount_tx(tx, fAccurate)
    return count

def get_legacy_sigopcount_tx(tx, fAccurate=True):
    count = 0
    for i in tx.vout:
        count += i.scriptPubKey.GetSigOpCount(fAccurate)
    for j in tx.vin:
        # scriptSig might be of type bytes, so convert to CScript for the moment
        count += CScript(j.scriptSig).GetSigOpCount(fAccurate)
    return count

# Create a scriptPubKey corresponding to either a short P2WSH output
# for the given pubkey, or a long P2WSH output of a 1-of-1 multisig
# for the given pubkey.  Returns the hex encoding of the scriptPubKey.
def witness_script(use_p2wsh, pubkey):
    if (use_p2wsh == False):
        # short P2WSH w/ P2PK
        witness_program = b'\x00' + CScript([hex_str_to_bytes(pubkey), OP_CHECKSIG])
        scripthash = ripemd160(hash256(witness_program))
        pkscript = CScript([OP_0, scripthash])
    else:
        # long P2WSH w/ 1-of-1 multisig
        witness_program = b'\x00' + CScript([OP_1, hex_str_to_bytes(pubkey), OP_1, OP_CHECKMULTISIG])
        scripthash = hash256(witness_program)
        pkscript = CScript([OP_0, scripthash])
    return bytes_to_hex_str(pkscript)

# Return a transaction (in hex) that spends the given utxo to a segwit output,
# optionally wrapping the segwit output using P2SH.
def create_witness_tx(node, use_p2wsh, utxo, pubkey, encode_p2sh, amount):
    if use_p2wsh:
        program = CScript([OP_1, hex_str_to_bytes(pubkey), OP_1, OP_CHECKMULTISIG])
        addr = script_to_p2sh_p2wsh(program) if encode_p2sh else script_to_p2wsh(program)
    else:
        addr = key_to_p2sh_p2wpk(pubkey) if encode_p2sh else key_to_p2wpk(pubkey)
    if not encode_p2sh:
        assert_equal(node.validateaddress(addr)['scriptPubKey'], witness_script(use_p2wsh, pubkey))
    return node.createrawtransaction([utxo], {addr: amount})

# Create a transaction spending a given utxo to a segwit output corresponding
# to the given pubkey: use_p2wsh determines whether to use P2WPKH or P2WSH;
# encode_p2sh determines whether to wrap in P2SH.
# sign=True will have the given node sign the transaction.
# insert_redeem_script will be added to the scriptSig, if given.
def send_to_witness(use_p2wsh, node, utxo, pubkey, encode_p2sh, amount, sign=True, insert_redeem_script=""):
    tx_to_witness = create_witness_tx(node, use_p2wsh, utxo, pubkey, encode_p2sh, amount)
    if (sign):
        signed = node.signrawtransaction(tx_to_witness)
        assert("errors" not in signed or len(["errors"]) == 0)
        return node.sendrawtransaction(signed["hex"])
    else:
        if (insert_redeem_script):
            tx = FromHex(CTransaction(), tx_to_witness)
            tx.vin[0].scriptSig += CScript([hex_str_to_bytes(insert_redeem_script)])
            tx_to_witness = ToHex(tx)

    return node.sendrawtransaction(tx_to_witness)
