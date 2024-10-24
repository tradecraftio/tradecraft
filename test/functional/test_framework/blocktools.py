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
"""Utilities for manipulating blocks and transactions."""

import struct
import time
import unittest

from .address import (
    address_to_scriptpubkey,
    key_to_p2wpk,
    script_to_p2wsh,
)
from .messages import (
    CBlock,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    SEQUENCE_FINAL,
    fastHash256,
    hash256,
    ser_uint256,
    tx_from_hex,
    uint256_from_str,
)
from .script import (
    CScript,
    CScriptNum,
    CScriptOp,
    OP_1,
    OP_TRUE,
)
from .script_util import (
    key_to_p2pk_script,
    key_to_p2wpk_script,
    keys_to_multisig_script,
    script_to_p2wsh_script,
)
from .util import (
    assert_equal,
)

WITNESS_SCALE_FACTOR = 4
MAX_BLOCK_SIGOPS = 20000
MAX_BLOCK_SIGOPS_WEIGHT = MAX_BLOCK_SIGOPS * WITNESS_SCALE_FACTOR

# Genesis block time (regtest)
TIME_GENESIS_BLOCK = 1356123600

MAX_FUTURE_BLOCK_TIME = 2 * 60 * 60

# Coinbase transaction outputs can only be spent after this number of new blocks (network rule)
COINBASE_MATURITY = 100

# From BIP141
WITNESS_COMMITMENT_HEADER = b"\x4b\x4a\x49\x48"

NORMAL_GBT_REQUEST_PARAMS = {"rules": ["segwit","finaltx","auxpow"]}
VERSIONBITS_LAST_OLD_BLOCK_VERSION = 4
MIN_BLOCKS_TO_KEEP = 288


def create_block(hashprev=None, coinbase=None, ntime=None, *, version=None, tmpl=None, txlist=None):
    """Create a block (with regtest difficulty)."""
    block = CBlock()
    if tmpl is None:
        tmpl = {}
    block.nVersion = version or tmpl.get('version') or VERSIONBITS_LAST_OLD_BLOCK_VERSION
    block.nTime = ntime or tmpl.get('curtime') or int(time.time() + 600)
    block.hashPrevBlock = hashprev or int(tmpl['previousblockhash'], 0x10)
    if tmpl and not tmpl.get('bits') is None:
        block.nBits = struct.unpack('>I', bytes.fromhex(tmpl['bits']))[0]
    else:
        block.nBits = 0x207fffff  # difficulty retargeting is disabled in REGTEST chainparams
    if coinbase is None:
        coinbase = create_coinbase(height=tmpl['height'])
    block.vtx.append(coinbase)
    if txlist:
        for tx in txlist:
            if not hasattr(tx, 'calc_sha256'):
                tx = tx_from_hex(tx)
            block.vtx.append(tx)
    finaltx_prevout = tmpl.get('finaltx', {}).get('prevout', [])
    if finaltx_prevout and tmpl['height'] > 100:
        add_final_tx(finaltx_prevout, block)
    block.hashMerkleRoot = block.calc_merkle_root()
    block.calc_sha256()
    return block

def get_final_tx_info(node):
    try:
        finaltx_prevout = node.getblocktemplate({'rules':['finaltx','segwit','auxpow']})['finaltx']['prevout']
    except KeyError:
        finaltx_prevout = []
    return finaltx_prevout

def add_final_tx(info, block):
    finaltx = CTransaction()
    finaltx.nLockTime = block.vtx[0].nLockTime
    finaltx.lock_height = block.vtx[0].lock_height
    finaltx.vout.append(CTxOut(0, CScript([OP_TRUE])))
    for prevout in info:
        finaltx.vin.append(CTxIn(COutPoint(uint256_from_str(bytes.fromhex(prevout['txid'])[::-1]), prevout['vout']), CScript([]), 0xffffffff))
        finaltx.vout[-1].nValue += prevout['amount']
    finaltx.rehash()
    block.vtx.append(finaltx)
    block.hashMerkleRoot = block.calc_merkle_root()
    block.rehash()
    return [{
        'txid': finaltx.hash,
        'vout': 0,
        'amount': int(finaltx.vout[-1].nValue),
    }]

def get_witness_script(witness_root, witness_nonce):
    witness_path = 0x01
    if witness_nonce:
        witness_path = 0x02
        witness_root = uint256_from_str(fastHash256(ser_uint256(witness_root),
                                                    ser_uint256(witness_nonce)))
    output_data = bytes((witness_path,)) + ser_uint256(witness_root) + WITNESS_COMMITMENT_HEADER
    return CScript([output_data])

def add_witness_commitment(block, nonce=0):
    """Add a witness commitment to the block's coinbase transaction.

    According to BIP141, blocks with witness rules active must commit to the
    hash of all in-block transactions including witness."""
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

    # witness commitment is the last output in block-final tx
    block.vtx[-1].vout[-1].scriptPubKey = get_witness_script(witness_root, nonce)
    block.vtx[-1].rehash()
    block.hashMerkleRoot = block.calc_merkle_root()
    if block.aux_pow:
        block.aux_pow.commit_hash_merkle_root = block.calc_commit_merkle_root()
    block.rehash()

    return [
        dict(txid=block.vtx[-1].hash, vout=n, amount=int(vout.nValue))
        for n,vout in enumerate(block.vtx[-1].vout)]


def script_BIP34_coinbase_height(height):
    if height <= 16:
        res = CScriptOp.encode_op_n(height)
        # Append dummy to increase scriptSig size above 2 (see bad-cb-length consensus rule)
        return CScript([res, OP_1])
    return CScript([CScriptNum(height)])


def create_coinbase(height, pubkey=None, *, script_pubkey=None, extra_output_script=None, fees=0, nValue=50):
    """Create a coinbase transaction.

    If pubkey is passed in, the coinbase output will be a P2PK output;
    otherwise an anyone-can-spend output.

    If extra_output_script is given, make a 0-value output to that
    script. This is useful to pad block weight/sigops as needed. """
    coinbase = CTransaction()
    coinbase.vin.append(CTxIn(COutPoint(0, 0xffffffff), script_BIP34_coinbase_height(height), SEQUENCE_FINAL))
    coinbaseoutput = CTxOut()
    coinbaseoutput.nValue = nValue * COIN
    if nValue == 50:
        halvings = int(height / 150)  # regtest
        coinbaseoutput.nValue >>= halvings
        coinbaseoutput.nValue += fees
    if pubkey is not None:
        coinbaseoutput.scriptPubKey = key_to_p2pk_script(pubkey)
    elif script_pubkey is not None:
        coinbaseoutput.scriptPubKey = script_pubkey
    else:
        coinbaseoutput.scriptPubKey = CScript([OP_TRUE])
    coinbase.vout = [coinbaseoutput]
    if extra_output_script is not None:
        coinbaseoutput2 = CTxOut()
        coinbaseoutput2.nValue = 0
        coinbaseoutput2.scriptPubKey = extra_output_script
        coinbase.vout.append(coinbaseoutput2)
    coinbase.lock_height = height
    coinbase.calc_sha256()
    return coinbase

def create_tx_with_script(prevtx, n, script_sig=b"", *, amount, script_pub_key=CScript()):
    """Return one-input, one-output transaction object
       spending the prevtx's n-th output with the given amount.

       Can optionally pass scriptPubKey and scriptSig, default is anyone-can-spend output.
    """
    tx = CTransaction()
    tx.lock_height = prevtx.lock_height
    assert n < len(prevtx.vout)
    tx.vin.append(CTxIn(COutPoint(prevtx.sha256, n), script_sig, SEQUENCE_FINAL))
    tx.vout.append(CTxOut(amount, script_pub_key))
    tx.calc_sha256()
    return tx

def get_legacy_sigopcount_block(block, accurate=True):
    count = 0
    for tx in block.vtx:
        count += get_legacy_sigopcount_tx(tx, accurate)
    return count

def get_legacy_sigopcount_tx(tx, accurate=True):
    count = 0
    for i in tx.vout:
        count += i.scriptPubKey.GetSigOpCount(accurate)
    for j in tx.vin:
        # scriptSig might be of type bytes, so convert to CScript for the moment
        count += CScript(j.scriptSig).GetSigOpCount(accurate)
    return count

def witness_script(use_p2wsh, pubkey):
    """Create a scriptPubKey for a pay-to-witness TxOut.

    This is either a short P2WSH output for the given pubkey, or a
    long P2WSH output of a 1-of-1 multisig for the given pubkey.
    Returns the hex encoding of the scriptPubKey.
    """
    if not use_p2wsh:
        # P2WPK instead
        pkscript = key_to_p2wpk_script(pubkey)
    else:
        # long P2WSH w/ 1-of-1 multisig
        witness_script = keys_to_multisig_script([pubkey])
        pkscript = script_to_p2wsh_script(witness_script)
    return pkscript.hex()

def create_witness_tx(node, use_p2wsh, utxo, pubkey, amount):
    """Return a transaction (in hex) that spends the given utxo to a segwit output.

    Optionally wrap the segwit output using P2SH."""
    if use_p2wsh:
        program = keys_to_multisig_script([pubkey])
        addr = script_to_p2wsh(program)
    else:
        addr = key_to_p2wpk(pubkey)
    assert_equal(address_to_scriptpubkey(addr).hex(), witness_script(use_p2wsh, pubkey))
    return node.createrawtransaction([utxo], {addr: amount})

def send_to_witness(use_p2wsh, node, utxo, pubkey, amount, sign=True, insert_redeem_script=""):
    """Create a transaction spending a given utxo to a segwit output.

    The output corresponds to the given pubkey: use_p2wsh determines whether to
    use P2WPK or P2WSH.
    sign=True will have the given node sign the transaction.
    insert_redeem_script will be added to the scriptSig, if given."""
    tx_to_witness = create_witness_tx(node, use_p2wsh, utxo, pubkey, amount)
    if (sign):
        signed = node.signrawtransactionwithwallet(tx_to_witness)
        assert "errors" not in signed or len(["errors"]) == 0
        return node.sendrawtransaction(signed["hex"])
    else:
        if (insert_redeem_script):
            tx = tx_from_hex(tx_to_witness)
            tx.vin[0].scriptSig += CScript([bytes.fromhex(insert_redeem_script)])
            tx_to_witness = tx.serialize().hex()

    return node.sendrawtransaction(tx_to_witness)

class TestFrameworkBlockTools(unittest.TestCase):
    def test_create_coinbase(self):
        height = 20
        coinbase_tx = create_coinbase(height=height)
        assert_equal(CScriptNum.decode(coinbase_tx.vin[0].scriptSig), height)
