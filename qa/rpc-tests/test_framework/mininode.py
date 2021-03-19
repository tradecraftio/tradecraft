#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-2016 The Freicoin developers
# Copyright (c) 2010-2021 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>

#
# mininode.py - Freicoin P2P network half-a-node
#
# This python code was modified from ArtForz' public domain  half-a-node, as
# found in the mini-node branch of http://github.com/jgarzik/pynode.
#
# NodeConn: an object which manages p2p connectivity to a freicoin node
# NodeConnCB: a base class that describes the interface for receiving
#             callbacks with network messages from a NodeConn
# CBlock, CTransaction, CBlockHeader, CTxIn, CTxOut, etc....:
#     data structures that should map to corresponding structures in
#     freicoin/primitives
# msg_block, msg_tx, msg_headers, etc.:
#     data structures that represent network messages
# ser_*, deser_*: functions that handle serialization/deserialization


import struct
import socket
import asyncore
import time
import sys
import random
from .util import hex_str_to_bytes, bytes_to_hex_str
from io import BytesIO
from codecs import encode
import hashlib
from threading import RLock
from threading import Thread
import logging
import copy
from test_framework.siphash import siphash256

BIP0031_VERSION = 60000
MY_VERSION = 70016  # include auxiliary proof-of-work
MY_SUBVERSION = b"/python-mininode-tester:0.0.3/"

MAX_INV_SZ = 50000
MAX_BLOCK_SIZE = 1000000

COIN = 100000000 # 1 frc in kria

NODE_NETWORK = (1 << 0)
NODE_GETUTXO = (1 << 1)
NODE_BLOOM = (1 << 2)
NODE_WITNESS = (1 << 3)

# Keep our own socket map for asyncore, so that we can track disconnects
# ourselves (to workaround an issue with closing an asyncore socket when
# using select)
mininode_socket_map = dict()

# One lock for synchronizing all data access between the networking thread (see
# NetworkThread below) and the thread running the test logic.  For simplicity,
# NodeConn acquires this lock whenever delivering a message to to a NodeConnCB,
# and whenever adding anything to the send buffer (in send_message()).  This
# lock should be acquired in the thread running the test logic to synchronize
# access to any data shared with the NodeConnCB or NodeConn.
mininode_lock = RLock()

# Serialization/deserialization tools
def sha256(s):
    return hashlib.new('sha256', s).digest()

def ripemd160(s):
    return hashlib.new('ripemd160', s).digest()

def hash256(s):
    return sha256(sha256(s))

from .sha256 import SHA256
_FastHash256Tag = sha256(b'')
def fastHash256(l,r):
    return (SHA256()
         .update(_FastHash256Tag)
         .update(_FastHash256Tag)
         .update(l)
         .update(r)
        ).midstate()[0]

def ser_compact_size(l):
    r = b""
    if l < 253:
        r = struct.pack("B", l)
    elif l < 0x10000:
        r = struct.pack("<BH", 253, l)
    elif l < 0x100000000:
        r = struct.pack("<BI", 254, l)
    else:
        r = struct.pack("<BQ", 255, l)
    return r

def deser_compact_size(f):
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    return nit

def ser_varint(n):
    r = b""
    while True:
        r += struct.pack("B", (n & 0x7F) | (0x80 if len(r) > 0 else 0x00))
        if n <= 0x7f:
            break
        n = (n >> 7) - 1
    return r[::-1]

def deser_varint(f):
    n = 0
    while True:
        c = struct.unpack("B", f.read(1))[0]
        n = (n << 7) | (c & 0x7F)
        if c & 0x80:
            n += 1
        else:
            return n

def deser_string(f):
    nit = deser_compact_size(f)
    return f.read(nit)

def ser_string(s):
    return ser_compact_size(len(s)) + s

def deser_uint256(f):
    r = 0
    for i in range(8):
        t = struct.unpack("<I", f.read(4))[0]
        r += t << (i * 32)
    return r


def ser_uint256(u):
    rs = b""
    for i in range(8):
        rs += struct.pack("<I", u & 0xFFFFFFFF)
        u >>= 32
    return rs


def uint256_from_str(s):
    r = 0
    t = struct.unpack("<IIIIIIII", s[:32])
    for i in range(8):
        r += t[i] << (i * 32)
    return r


def uint256_from_compact(c):
    nbytes = (c >> 24) & 0xFF
    v = (c & 0xFFFFFF) << (8 * (nbytes - 3))
    return v


def deser_vector(f, c):
    nit = deser_compact_size(f)
    r = []
    for i in range(nit):
        t = c()
        t.deserialize(f)
        r.append(t)
    return r


# ser_function_name: Allow for an alternate serialization function on the
# entries in the vector (we use this for serializing the vector of transactions
# for a witness block).
def ser_vector(l, ser_function_name=None):
    r = ser_compact_size(len(l))
    for i in l:
        if ser_function_name:
            r += getattr(i, ser_function_name)()
        else:
            r += i.serialize()
    return r


def deser_uint256_vector(f):
    nit = deser_compact_size(f)
    r = []
    for i in range(nit):
        t = deser_uint256(f)
        r.append(t)
    return r


def ser_uint256_vector(l):
    r = ser_compact_size(len(l))
    for i in l:
        r += ser_uint256(i)
    return r


def deser_string_vector(f):
    nit = deser_compact_size(f)
    r = []
    for i in range(nit):
        t = deser_string(f)
        r.append(t)
    return r


def ser_string_vector(l):
    r = ser_compact_size(len(l))
    for sv in l:
        r += ser_string(sv)
    return r


def deser_int_vector(f):
    nit = deser_compact_size(f)
    r = []
    for i in range(nit):
        t = struct.unpack("<i", f.read(4))[0]
        r.append(t)
    return r


def ser_int_vector(l):
    r = ser_compact_size(len(l))
    for i in l:
        r += struct.pack("<i", i)
    return r

# Deserialize from a hex string representation (eg from RPC)
def FromHex(obj, hex_string):
    obj.deserialize(BytesIO(hex_str_to_bytes(hex_string)))
    return obj

# Convert a binary-serializable object to hex (eg for submission via RPC)
def ToHex(obj):
    return bytes_to_hex_str(obj.serialize())

# Objects that map to freicoind objects, which can be serialized/deserialized

class CAddress(object):
    def __init__(self):
        self.nServices = 1
        self.pchReserved = b"\x00" * 10 + b"\xff" * 2
        self.ip = "0.0.0.0"
        self.port = 0

    def deserialize(self, f):
        self.nServices = struct.unpack("<Q", f.read(8))[0]
        self.pchReserved = f.read(12)
        self.ip = socket.inet_ntoa(f.read(4))
        self.port = struct.unpack(">H", f.read(2))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.nServices)
        r += self.pchReserved
        r += socket.inet_aton(self.ip)
        r += struct.pack(">H", self.port)
        return r

    def __repr__(self):
        return "CAddress(nServices=%i ip=%s port=%i)" % (self.nServices,
                                                         self.ip, self.port)

MSG_WITNESS_FLAG = 1<<30

class CInv(object):
    typemap = {
        0: "Error",
        1: "TX",
        2: "Block",
        1|MSG_WITNESS_FLAG: "WitnessTx",
        2|MSG_WITNESS_FLAG : "WitnessBlock",
        4: "CompactBlock"
    }

    def __init__(self, t=0, h=0):
        self.type = t
        self.hash = h

    def deserialize(self, f):
        self.type = struct.unpack("<i", f.read(4))[0]
        self.hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.type)
        r += ser_uint256(self.hash)
        return r

    def __repr__(self):
        return "CInv(type=%s hash=%064x)" \
            % (self.typemap[self.type], self.hash)


class CBlockLocator(object):
    def __init__(self):
        self.nVersion = MY_VERSION
        self.vHave = []

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.vHave = deser_uint256_vector(f)

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += ser_uint256_vector(self.vHave)
        return r

    def __repr__(self):
        return "CBlockLocator(nVersion=%i vHave=%s)" \
            % (self.nVersion, repr(self.vHave))


class COutPoint(object):
    def __init__(self, hash=0, n=0):
        self.hash = hash
        self.n = n

    def deserialize(self, f):
        self.hash = deser_uint256(f)
        self.n = struct.unpack("<I", f.read(4))[0]

    def serialize(self):
        r = b""
        r += ser_uint256(self.hash)
        r += struct.pack("<I", self.n)
        return r

    def __repr__(self):
        return "COutPoint(hash=%064x n=%i)" % (self.hash, self.n)


class CTxIn(object):
    def __init__(self, outpoint=None, scriptSig=b"", nSequence=0):
        if outpoint is None:
            self.prevout = COutPoint()
        else:
            self.prevout = outpoint
        self.scriptSig = scriptSig
        self.nSequence = nSequence

    def deserialize(self, f):
        self.prevout = COutPoint()
        self.prevout.deserialize(f)
        self.scriptSig = deser_string(f)
        self.nSequence = struct.unpack("<I", f.read(4))[0]

    def serialize(self):
        r = b""
        r += self.prevout.serialize()
        r += ser_string(self.scriptSig)
        r += struct.pack("<I", self.nSequence)
        return r

    def __repr__(self):
        return "CTxIn(prevout=%s scriptSig=%s nSequence=%i)" \
            % (repr(self.prevout), bytes_to_hex_str(self.scriptSig),
               self.nSequence)


class CTxOut(object):
    def __init__(self, nValue=0, scriptPubKey=b""):
        self.nValue = nValue
        self.scriptPubKey = scriptPubKey

    def deserialize(self, f):
        self.nValue = struct.unpack("<q", f.read(8))[0]
        self.scriptPubKey = deser_string(f)

    def serialize(self):
        r = b""
        r += struct.pack("<q", self.nValue)
        r += ser_string(self.scriptPubKey)
        return r

    def __repr__(self):
        return "CTxOut(nValue=%i.%08i scriptPubKey=%s)" \
            % (self.nValue // COIN, self.nValue % COIN,
               bytes_to_hex_str(self.scriptPubKey))


class CScriptWitness(object):
    def __init__(self):
        # stack is a vector of strings
        self.stack = []

    def __repr__(self):
        return "CScriptWitness(%s)" % \
               (",".join([bytes_to_hex_str(x) for x in self.stack]))

    def is_null(self):
        if self.stack:
            return False
        return True


class CTxInWitness(object):
    def __init__(self):
        self.scriptWitness = CScriptWitness()

    def deserialize(self, f):
        self.scriptWitness.stack = deser_string_vector(f)

    def serialize(self):
        return ser_string_vector(self.scriptWitness.stack)

    def __repr__(self):
        return repr(self.scriptWitness)

    def is_null(self):
        return self.scriptWitness.is_null()


class CTxWitness(object):
    def __init__(self):
        self.vtxinwit = []

    def deserialize(self, f):
        for i in range(len(self.vtxinwit)):
            self.vtxinwit[i].deserialize(f)

    def serialize(self):
        r = b""
        # This is different than the usual vector serialization --
        # we omit the length of the vector, which is required to be
        # the same length as the transaction's vin vector.
        for x in self.vtxinwit:
            r += x.serialize()
        return r

    def __repr__(self):
        return "CTxWitness(%s)" % \
               (';'.join([repr(x) for x in self.vtxinwit]))

    def is_null(self):
        for x in self.vtxinwit:
            if not x.is_null():
                return False
        return True


class CTransaction(object):
    def __init__(self, tx=None):
        if tx is None:
            self.nVersion = 2
            self.vin = []
            self.vout = []
            self.wit = CTxWitness()
            self.nLockTime = 0
            self.lock_height = 0
            self.sha256 = None
            self.hash = None
        else:
            self.nVersion = tx.nVersion
            self.vin = copy.deepcopy(tx.vin)
            self.vout = copy.deepcopy(tx.vout)
            self.nLockTime = tx.nLockTime
            self.lock_height = tx.lock_height
            self.sha256 = tx.sha256
            self.hash = tx.hash
            self.wit = copy.deepcopy(tx.wit)

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        flags = 0
        dummy = struct.unpack("<B", f.read(1))[0]
        if dummy == 255:
            flags = struct.unpack("<B", f.read(1))[0]
            self.vin = deser_vector(f, CTxIn)
        else:
            if dummy <= 252:
                size = dummy
            if dummy == 253:
                size = struct.unpack("<H", f.read(2))[0]
            if dummy == 254:
                size = struct.unpack("<I", f.read(4))[0]
            self.vin = []
            for _ in range(size):
                txin = CTxIn()
                txin.deserialize(f)
                self.vin.append(txin)
        self.vout = deser_vector(f, CTxOut)
        if flags != 0:
            self.wit.vtxinwit = [CTxInWitness() for i in range(len(self.vin))]
            self.wit.deserialize(f)
        self.nLockTime = struct.unpack("<I", f.read(4))[0]
        if self.nVersion!=1 or len(self.vin)!=1 or self.vin[0].prevout.hash!=0 or self.vin[0].prevout.n not in (-1,0xffffffff):
            self.lock_height = struct.unpack("<I", f.read(4))[0]
        self.sha256 = None
        self.hash = None

    def serialize_without_witness(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += ser_vector(self.vin)
        r += ser_vector(self.vout)
        r += struct.pack("<I", self.nLockTime)
        if self.nVersion!=1 or len(self.vin)!=1 or self.vin[0].prevout.hash!=0 or self.vin[0].prevout.n not in (-1,0xffffffff):
            r += struct.pack("<I", self.lock_height)
        return r

    # Only serialize with witness when explicitly called for
    def serialize_with_witness(self):
        flags = 0
        if not self.wit.is_null():
            flags |= 1
        r = b""
        r += struct.pack("<i", self.nVersion)
        if flags:
            r += struct.pack("<B", 255)
            r += struct.pack("<B", flags)
        r += ser_vector(self.vin)
        r += ser_vector(self.vout)
        if flags & 1:
            if (len(self.wit.vtxinwit) != len(self.vin)):
                # vtxinwit must have the same length as vin
                self.wit.vtxinwit = self.wit.vtxinwit[:len(self.vin)]
                for i in range(len(self.wit.vtxinwit), len(self.vin)):
                    self.wit.vtxinwit.append(CTxInWitness())
            r += self.wit.serialize()
        r += struct.pack("<I", self.nLockTime)
        if self.nVersion!=1 or len(self.vin)!=1 or self.vin[0].prevout.hash!=0 or self.vin[0].prevout.n not in (-1,0xffffffff):
            r += struct.pack("<I", self.lock_height)
        return r

    # Regular serialization is without witness -- must explicitly
    # call serialize_with_witness to include witness data.
    def serialize(self):
        return self.serialize_without_witness()

    # Recalculate the txid (transaction hash without witness)
    def rehash(self):
        self.sha256 = None
        self.calc_sha256()

    # We will only cache the serialization without witness in
    # self.sha256 and self.hash -- those are expected to be the txid.
    def calc_sha256(self, with_witness=False):
        if with_witness:
            # Don't cache the result, just return it
            return uint256_from_str(hash256(self.serialize_with_witness()))

        if self.sha256 is None:
            self.sha256 = uint256_from_str(hash256(self.serialize_without_witness()))
        self.hash = encode(hash256(self.serialize())[::-1], 'hex_codec').decode('ascii')

    def is_valid(self):
        self.calc_sha256()
        for tout in self.vout:
            if tout.nValue < 0 or tout.nValue > 21000000 * COIN:
                return False
        return True

    def __repr__(self):
        return "CTransaction(nVersion=%i vin=%s vout=%s wit=%s nLockTime=%i, lock_height=%i)" \
            % (self.nVersion, repr(self.vin), repr(self.vout), repr(self.wit), self.nLockTime, self.lock_height)


class AuxProofOfWork(object):
    def __init__(self, aux_pow=None):
        if aux_pow is None:
            self.set_null()
        else:
            self.commit_version = aux_pow.commit_version
            self.commit_hash_merkle_root = aux_pow.commit_hash_merkle_root
            self.commit_time = aux_pow.commit_time
            self.commit_bits = aux_pow.commit_bits
            self.commit_nonce = aux_pow.commit_nonce
            self.secret_lo = aux_pow.secret_lo
            self.secret_hi = aux_pow.secret_hi
            self.midstate_hash = aux_pow.midstate_hash
            self.midstate_buffer = aux_pow.midstate_buffer
            self.midstate_length = aux_pow.midstate_length
            self.aux_lock_time = aux_pow.aux_lock_time
            self.aux_branch = aux_pow.aux_branch
            self.aux_num_txns = aux_pow.aux_num_txns
            self.aux_version = aux_pow.aux_version
            self.aux_hash_prev_block = aux_pow.aux_hash_prev_block
            self.aux_bits = aux_pow.aux_bits
            self.aux_nonce = aux_pow.aux_nonce

    def set_null(self):
        self.commit_version = 0
        self.commit_hash_merkle_root = 0
        self.commit_time = 0
        self.commit_bits = 0
        self.commit_nonce = 0
        self.secret_lo = 0
        self.secret_hi = 0
        self.midstate_hash = 0
        self.midstate_buffer = b''
        self.midstate_length = 0
        self.aux_lock_time = 0
        self.aux_branch = []
        self.aux_num_txns = 0
        self.aux_version = 0
        self.aux_hash_prev_block = 0
        self.aux_bits = 0
        self.aux_nonce = 0

    def __bool__(self):
        return bool(self.aux_num_txns)
    __nonzero__ = __bool__

    def deserialize(self, f):
        self.commit_version = struct.unpack("<I", f.read(4))[0]
        self.commit_hash_merkle_root = deser_uint256(f)
        self.commit_time = struct.unpack("<I", f.read(4))[0]
        self.commit_bits = struct.unpack("<I", f.read(4))[0]
        self.commit_nonce = struct.unpack("<I", f.read(4))[0]
        self.secret_lo = struct.unpack("<Q", f.read(8))[0]
        self.secret_hi = struct.unpack("<Q", f.read(8))[0]
        self.midstate_hash = deser_uint256(f)
        self.midstate_buffer = deser_string(f)
        self.midstate_length = deser_varint(f)
        self.aux_lock_time = struct.unpack("<I", f.read(4))[0]
        self.aux_branch = deser_uint256_vector(f)
        self.aux_num_txns = deser_varint(f) + 1
        self.aux_version = struct.unpack("<I", f.read(4))[0]
        self.aux_hash_prev_block = deser_uint256(f)
        self.aux_bits = struct.unpack("<I", f.read(4))[0]
        self.aux_nonce = struct.unpack("<I", f.read(4))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<I", self.commit_version)
        r += ser_uint256(self.commit_hash_merkle_root)
        r += struct.pack("<I", self.commit_time)
        r += struct.pack("<I", self.commit_bits)
        r += struct.pack("<I", self.commit_nonce)
        r += struct.pack("<Q", self.secret_lo)
        r += struct.pack("<Q", self.secret_hi)
        r += ser_uint256(self.midstate_hash)
        r += ser_string(self.midstate_buffer)
        r += ser_varint(self.midstate_length)
        r += struct.pack("<I", self.aux_lock_time)
        r += ser_uint256_vector(self.aux_branch)
        assert(self.aux_num_txns > 0)
        r += ser_varint(self.aux_num_txns - 1)
        r += struct.pack("<I", self.aux_version)
        r += ser_uint256(self.aux_hash_prev_block)
        r += struct.pack("<I", self.aux_bits)
        r += struct.pack("<I", self.aux_nonce)
        return r

    def __repr__(self):
        return "AuxProofOfWork(commit_version=%d commit_hash_merkle_root=%064x commit_time=%s commit_bits=%08x commit_nonce=%08x secret=%08x%08x midstate_hash=%064x midstate_buffer=%s midstate_length=%d aux_lock_time=%08x aux_branch=%s aux_num_txns=%d aux_version=%d aux_hash_prev_block=%064x aux_bits=%08x aux_nonce=%08x)" \
            % (self.commit_version, self.commit_hash_merkle_root,
               time.ctime(self.commit_time), self.commit_bits,
               self.commit_nonce, self.secret_hi, self.secret_lo,
               self.midstate_hash,
               "".join(["%02x"%c for c in self.midstate_buffer]),
               self.midstate_length, self.aux_lock_time,
               repr(self.aux_branch), self.aux_num_txns,
               self.aux_version, self.aux_hash_prev_block,
               self.aux_bits, self.aux_nonce)

class CBlockHeader(object):
    def __init__(self, header=None):
        if header is None:
            self.set_null()
        else:
            self.nVersion = header.nVersion
            self.hashPrevBlock = header.hashPrevBlock
            self.hashMerkleRoot = header.hashMerkleRoot
            self.nTime = header.nTime
            self.nBits = header.nBits
            self.nNonce = header.nNonce
            self.aux_pow = header.aux_pow
            self.aux_hash1 = header.aux_hash1
            self.aux_hash2 = header.aux_hash2
            self.calc_auxiliary_hash()
            self.sha256 = header.sha256
            self.hash = header.hash
            self.calc_sha256()

    def set_null(self):
        self.nVersion = 1
        self.hashPrevBlock = 0
        self.hashMerkleRoot = 0
        self.nTime = 0
        self.nBits = 0
        self.nNonce = 0
        self.aux_pow = False
        self.aux_hash1 = None
        self.aux_hash2 = None
        self.sha256 = None
        self.hash = None

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.hashPrevBlock = deser_uint256(f)
        self.hashMerkleRoot = deser_uint256(f)
        self.nTime = struct.unpack("<I", f.read(4))[0]
        self.nBits = struct.unpack("<I", f.read(4))[0]
        extended = False
        if self.nBits & 0x00800000:
            self.nBits = self.nBits ^ 0x00800000
            extended = True
        self.nNonce = struct.unpack("<I", f.read(4))[0]
        self.aux_pow = False
        if extended:
            dummy = struct.unpack("B", f.read(1))[0]
            assert(dummy == 0xff)
            flags = struct.unpack("B", f.read(1))[0]
            if flags & 1:
                flags = flags ^ 1
                self.aux_pow = AuxProofOfWork()
                self.aux_pow.deserialize(f)
            assert(flags == 0x00)
        self.sha256 = None
        self.hash = None

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += ser_uint256(self.hashPrevBlock)
        r += ser_uint256(self.hashMerkleRoot)
        r += struct.pack("<I", self.nTime)
        bits = self.nBits
        if self.aux_pow:
            bits = bits | 0x00800000
        r += struct.pack("<I", bits)
        r += struct.pack("<I", self.nNonce)
        if self.aux_pow:
            r += struct.pack("BB", 0xff, 0x01)
            r += self.aux_pow.serialize()
        return r

    def calc_auxiliary_hash(self):
        if not self.aux_pow:
            # Nothing to do
            self.aux_hash1 = None
            self.aux_hash2 = None
            return

        blkhdr = CBlockHeader()
        blkhdr.nVersion       = self.aux_pow.commit_version;
        blkhdr.hashPrevBlock  = self.hashPrevBlock;
        blkhdr.hashMerkleRoot = self.aux_pow.commit_hash_merkle_root;
        blkhdr.nTime          = self.aux_pow.commit_time;
        blkhdr.nBits          = self.aux_pow.commit_bits;
        blkhdr.nNonce         = self.aux_pow.commit_nonce;

        if self.aux_hash1 is None:
            # Calculate the hash of the block template.
            blkhdr.sha256 = None
            blkhdr.calc_sha256()
            h = ser_uint256(blkhdr.sha256)

            # Combine with the hash of the block-witholding secret.
            secret = b""
            secret += struct.pack("<Q", self.aux_pow.secret_lo)
            secret += struct.pack("<Q", self.aux_pow.secret_hi)
            h = fastHash256(h, hash256(secret))

            # Resume the hash writer state for the coinbase.
            midstate = SHA256()
            midstate.state = SHA256.State(*struct.unpack(">LLLLLLLL", ser_uint256(self.aux_pow.midstate_hash)))
            midstate.length = self.aux_pow.midstate_length * 8
            midstate.buffer = self.aux_pow.midstate_buffer
            midstate.round_offset = (midstate.length // 512) * 64

            # Add the block template commitment and tx.nLockTime
            midstate.update(h)
            midstate.update(bytes([0x4b, 0x4a, 0x49, 0x48]))
            midstate.update(struct.pack("<I", self.aux_pow.aux_lock_time))
            h = sha256(midstate.digest())

            # Calculate the auxiliary hashMerkleRoot
            n = self.aux_pow.aux_num_txns - 1
            for h2 in self.aux_pow.aux_branch:
                while n and not (n & 1):
                    h = fastHash256(h, h)
                    n = n >> 1
                h = fastHash256(h2, h)

            # Complete the auxiliary block header
            blkhdraux = CBlockHeader()
            blkhdraux.nVersion       = self.aux_pow.aux_version
            blkhdraux.hashPrevBlock  = self.aux_pow.aux_hash_prev_block
            blkhdraux.hashMerkleRoot = uint256_from_str(h)
            blkhdraux.nTime          = self.nTime
            blkhdraux.nBits          = self.aux_pow.aux_bits
            blkhdraux.nNonce         = self.aux_pow.aux_nonce

            blkhdraux.sha256 = None
            blkhdraux.calc_sha256()
            self.aux_hash1 = blkhdraux.sha256

        if self.aux_hash2 is None:
            # The 2nd stage hash is H(secret || blkhdr || aux_hash1)
            r = b""
            r += struct.pack("<Q", self.aux_pow.secret_lo)
            r += struct.pack("<Q", self.aux_pow.secret_hi)
            r += blkhdr.serialize()
            r += ser_uint256(self.aux_hash1)
            assert(len(r) == 128)

            # The 2nd stage hash is raw sha256--no padding!
            self.aux_hash2 = uint256_from_str(SHA256(r).midstate()[0])

    def calc_sha256(self):
        if self.sha256 is None:
            r = b""
            r += struct.pack("<i", self.nVersion)
            r += ser_uint256(self.hashPrevBlock)
            r += ser_uint256(self.hashMerkleRoot)
            r += struct.pack("<I", self.nTime)
            r += struct.pack("<I", self.nBits)
            r += struct.pack("<I", self.nNonce)
            self.sha256 = uint256_from_str(hash256(r))
            self.hash = encode(hash256(r)[::-1], 'hex_codec').decode('ascii')

    def rehash(self):
        self.aux_hash1 = None
        self.aux_hash2 = None
        self.calc_auxiliary_hash()
        self.sha256 = None
        self.calc_sha256()
        return self.sha256

    def __repr__(self):
        aux_pow = ""
        if self.aux_pow:
            aux_pow = " aux_pow=%s" % repr(self.aux_pow)
        return "CBlockHeader(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x%s)" \
            % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
               time.ctime(self.nTime), self.nBits, self.nNonce, aux_pow)


class CBlock(CBlockHeader):
    def __init__(self, header=None):
        super(CBlock, self).__init__(header)
        self.vtx = []

    def deserialize(self, f):
        super(CBlock, self).deserialize(f)
        self.vtx = deser_vector(f, CTransaction)

    def serialize(self, with_witness=False):
        r = b""
        r += super(CBlock, self).serialize()
        if with_witness:
            r += ser_vector(self.vtx, "serialize_with_witness")
        else:
            r += ser_vector(self.vtx)
        return r

    def get_bias(self):
        return 0 if not self.aux_pow else (self.aux_pow.commit_nonce & 0xff)

    def set_bias(self, bias):
        if self.aux_pow:
            self.aux_pow.commit_nonce = (self.aux_pow.commit_nonce & 0xffffff00) | (bias & 0xff)

    # Calculate the merkle root given a vector of transaction hashes
    def get_merkle_root(self, hashes):
        while len(hashes) > 1:
            newhashes = []
            for i in range(0, len(hashes), 2):
                i2 = min(i+1, len(hashes)-1)
                newhashes.append(hash256(hashes[i] + hashes[i2]))
            hashes = newhashes
        return uint256_from_str(hashes[0])

    # Calculate the Merkle root using fast Merkle trees, given a
    # vector of transaction hashes.
    def get_fast_merkle_root(self, hashes):
        while len(hashes) > 1:
            newhashes = []
            for i in range(0, len(hashes), 2):
                if i < len(hashes)-1:
                    newhashes.append(fastHash256(hashes[i], hashes[i+1]))
                else:
                    newhashes.append(hashes[i])
            hashes = newhashes
        return uint256_from_str(hashes[0])

    def calc_merkle_root(self):
        hashes = []
        for tx in self.vtx:
            tx.calc_sha256()
            hashes.append(ser_uint256(tx.sha256))
        return self.get_merkle_root(hashes)

    def calc_commit_merkle_root(self):
        hashes = []
        for tx in self.vtx:
            tx.calc_sha256()
            hashes.append(ser_uint256(tx.sha256))
        cb = CTransaction(self.vtx[0])
        cb.vin[0].scriptSig = b""
        cb.vin[0].nSequence = 0
        cb.rehash()
        hashes[0] = ser_uint256(cb.sha256)
        return self.get_merkle_root(hashes)

    def calc_witness_merkle_root(self):
        # For witness root purposes, the hash of the coinbase does not
        # include the coinbase witness, which is the witness nonce,
        # and it has both the coinbase string (scriptSig) and
        # nSequence fields of the coinbase input zero'd out.
        cb = self.vtx[0].serialize_without_witness()
        pos = (4  # nVersion
            +  1  # len(vin)
            + 32  # vin[0].prevout.hash
            +  4) # vin[0].prevout.n
        if len(cb) >= (pos + 1):
            pos2 = pos + 1 + cb[pos]
            if len(cb) >= (pos2 + 4):
                cb = cb[:pos] + b'\x00' + (4 * b'\x00') + cb[pos2+4:]
        hashes = [hash256(cb)]

        for tx in self.vtx[1:-1]:
            # Calculate the hashes with witness data
            hashes.append(ser_uint256(tx.calc_sha256(True)))

        # Strip the witness commitment from the block-final transaction. The
        # witness is not serialized, but the block-final transaction never has a
        # witness anyway.
        tx = self.vtx[-1].serialize_without_witness()
        if len(tx) >= (1 + 32 + 4 + 8) and tx[-8-4]==0x4b and tx[-8-3]==0x4a and tx[-8-2]==0x49 and tx[-8-1]==0x48:
            tx = tx[:-8-4-32-1] + (33 * b'\x00') + tx[-8-4:]
        hashes.append(hash256(tx))

        return self.get_fast_merkle_root(hashes)

    def is_valid(self):
        self.calc_sha256()
        target = uint256_from_compact(self.nBits)
        if self.sha256 > target:
            return False
        for tx in self.vtx:
            if not tx.is_valid():
                return False
        if self.calc_merkle_root() != self.hashMerkleRoot:
            return False
        return True

    def setup_default_aux_pow(self):
        self.aux_pow = AuxProofOfWork()
        self.aux_pow.commit_version = self.nVersion
        self.aux_pow.commit_hash_merkle_root = self.calc_commit_merkle_root()
        self.aux_pow.commit_bits = 0x207fffff # No difficulty adjustment on regtest
        self.aux_pow.midstate_hash = uint256_from_str(SHA256().midstate()[0])
        self.aux_pow.aux_num_txns = 1
        self.aux_pow.aux_version = 0x20000000
        self.aux_pow.aux_bits = self.aux_pow.commit_bits

    def solve(self):
        if self.aux_pow:
            self.rehash()
            aux_target1 = uint256_from_compact(self.aux_pow.commit_bits)
            aux_target2 = 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
            bias = self.get_bias()
            aux_target1 = aux_target1 << bias
            assert (aux_target1 <= aux_target2)
            aux_target2 = aux_target2 >> bias
            while (self.aux_hash1 > aux_target1) or (self.aux_hash2 > aux_target2):
                self.aux_pow.aux_nonce += 1
                self.rehash()
            # First remove the existing commitment, if there is one.  There
            # isn't a way to identify the commitment, so we presume that one is
            # present if the coinbase miner string is large enough to contain
            # it.
            if len(self.vtx[0].vin[0].scriptSig) >= 32:
                self.vtx[0].vin[0].scriptSig = self.vtx[0].vin[0].scriptSig[:-32]
            self.vtx[0].vin[0].scriptSig += ser_uint256(self.aux_hash2)
            self.vtx[0].rehash()
            self.hashMerkleRoot = self.calc_merkle_root()
        self.rehash()
        target = uint256_from_compact(self.nBits)
        while self.sha256 > target:
            self.nNonce += 1
            self.rehash()

    def __repr__(self):
        aux_pow_str = ""
        if self.aux_pow:
            aux_pow_str = " aux_pow=%s" % repr(self.aux_pow)
        return "CBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x%s vtx=%s)" \
            % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
               time.ctime(self.nTime), self.nBits, self.nNonce, aux_pow_str,
               repr(self.vtx))


class CUnsignedAlert(object):
    def __init__(self):
        self.nVersion = 1
        self.nRelayUntil = 0
        self.nExpiration = 0
        self.nID = 0
        self.nCancel = 0
        self.setCancel = []
        self.nMinVer = 0
        self.nMaxVer = 0
        self.setSubVer = []
        self.nPriority = 0
        self.strComment = b""
        self.strStatusBar = b""
        self.strReserved = b""

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.nRelayUntil = struct.unpack("<q", f.read(8))[0]
        self.nExpiration = struct.unpack("<q", f.read(8))[0]
        self.nID = struct.unpack("<i", f.read(4))[0]
        self.nCancel = struct.unpack("<i", f.read(4))[0]
        self.setCancel = deser_int_vector(f)
        self.nMinVer = struct.unpack("<i", f.read(4))[0]
        self.nMaxVer = struct.unpack("<i", f.read(4))[0]
        self.setSubVer = deser_string_vector(f)
        self.nPriority = struct.unpack("<i", f.read(4))[0]
        self.strComment = deser_string(f)
        self.strStatusBar = deser_string(f)
        self.strReserved = deser_string(f)

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += struct.pack("<q", self.nRelayUntil)
        r += struct.pack("<q", self.nExpiration)
        r += struct.pack("<i", self.nID)
        r += struct.pack("<i", self.nCancel)
        r += ser_int_vector(self.setCancel)
        r += struct.pack("<i", self.nMinVer)
        r += struct.pack("<i", self.nMaxVer)
        r += ser_string_vector(self.setSubVer)
        r += struct.pack("<i", self.nPriority)
        r += ser_string(self.strComment)
        r += ser_string(self.strStatusBar)
        r += ser_string(self.strReserved)
        return r

    def __repr__(self):
        return "CUnsignedAlert(nVersion %d, nRelayUntil %d, nExpiration %d, nID %d, nCancel %d, nMinVer %d, nMaxVer %d, nPriority %d, strComment %s, strStatusBar %s, strReserved %s)" \
            % (self.nVersion, self.nRelayUntil, self.nExpiration, self.nID,
               self.nCancel, self.nMinVer, self.nMaxVer, self.nPriority,
               self.strComment, self.strStatusBar, self.strReserved)


class CAlert(object):
    def __init__(self):
        self.vchMsg = b""
        self.vchSig = b""

    def deserialize(self, f):
        self.vchMsg = deser_string(f)
        self.vchSig = deser_string(f)

    def serialize(self):
        r = b""
        r += ser_string(self.vchMsg)
        r += ser_string(self.vchSig)
        return r

    def __repr__(self):
        return "CAlert(vchMsg.sz %d, vchSig.sz %d)" \
            % (len(self.vchMsg), len(self.vchSig))


class PrefilledTransaction(object):
    def __init__(self, index=0, tx = None):
        self.index = index
        self.tx = tx

    def deserialize(self, f):
        self.index = deser_compact_size(f)
        self.tx = CTransaction()
        self.tx.deserialize(f)

    def serialize(self, with_witness=False):
        r = b""
        r += ser_compact_size(self.index)
        if with_witness:
            r += self.tx.serialize_with_witness()
        else:
            r += self.tx.serialize_without_witness()
        return r

    def serialize_with_witness(self):
        return self.serialize(with_witness=True)

    def __repr__(self):
        return "PrefilledTransaction(index=%d, tx=%s)" % (self.index, repr(self.tx))

# This is what we send on the wire, in a cmpctblock message.
class P2PHeaderAndShortIDs(object):
    def __init__(self):
        self.header = CBlockHeader()
        self.nonce = 0
        self.shortids_length = 0
        self.shortids = []
        self.prefilled_txn_length = 0
        self.prefilled_txn = []

    def deserialize(self, f):
        self.header.deserialize(f)
        self.nonce = struct.unpack("<Q", f.read(8))[0]
        self.shortids_length = deser_compact_size(f)
        for i in range(self.shortids_length):
            # shortids are defined to be 6 bytes in the spec, so append
            # two zero bytes and read it in as an 8-byte number
            self.shortids.append(struct.unpack("<Q", f.read(6) + b'\x00\x00')[0])
        self.prefilled_txn = deser_vector(f, PrefilledTransaction)
        self.prefilled_txn_length = len(self.prefilled_txn)

    # When using version 2 compact blocks, we must serialize with_witness.
    def serialize(self, with_witness=False):
        r = b""
        r += self.header.serialize()
        r += struct.pack("<Q", self.nonce)
        r += ser_compact_size(self.shortids_length)
        for x in self.shortids:
            # We only want the first 6 bytes
            r += struct.pack("<Q", x)[0:6]
        if with_witness:
            r += ser_vector(self.prefilled_txn, "serialize_with_witness")
        else:
            r += ser_vector(self.prefilled_txn)
        return r

    def __repr__(self):
        return "P2PHeaderAndShortIDs(header=%s, nonce=%d, shortids_length=%d, shortids=%s, prefilled_txn_length=%d, prefilledtxn=%s" % (repr(self.header), self.nonce, self.shortids_length, repr(self.shortids), self.prefilled_txn_length, repr(self.prefilled_txn))

# P2P version of the above that will use witness serialization (for compact
# block version 2)
class P2PHeaderAndShortWitnessIDs(P2PHeaderAndShortIDs):
    def serialize(self):
        return super(P2PHeaderAndShortWitnessIDs, self).serialize(with_witness=True)

# Calculate the BIP 152-compact blocks shortid for a given transaction hash
def calculate_shortid(k0, k1, tx_hash):
    expected_shortid = siphash256(k0, k1, tx_hash)
    expected_shortid &= 0x0000ffffffffffff
    return expected_shortid

# This version gets rid of the array lengths, and reinterprets the differential
# encoding into indices that can be used for lookup.
class HeaderAndShortIDs(object):
    def __init__(self, p2pheaders_and_shortids = None):
        self.header = CBlockHeader()
        self.nonce = 0
        self.shortids = []
        self.prefilled_txn = []
        self.use_witness = False

        if p2pheaders_and_shortids != None:
            self.header = p2pheaders_and_shortids.header
            self.nonce = p2pheaders_and_shortids.nonce
            self.shortids = p2pheaders_and_shortids.shortids
            last_index = -1
            for x in p2pheaders_and_shortids.prefilled_txn:
                self.prefilled_txn.append(PrefilledTransaction(x.index + last_index + 1, x.tx))
                last_index = self.prefilled_txn[-1].index

    def to_p2p(self):
        if self.use_witness:
            ret = P2PHeaderAndShortWitnessIDs()
        else:
            ret = P2PHeaderAndShortIDs()
        ret.header = self.header
        ret.nonce = self.nonce
        ret.shortids_length = len(self.shortids)
        ret.shortids = self.shortids
        ret.prefilled_txn_length = len(self.prefilled_txn)
        ret.prefilled_txn = []
        last_index = -1
        for x in self.prefilled_txn:
            ret.prefilled_txn.append(PrefilledTransaction(x.index - last_index - 1, x.tx))
            last_index = x.index
        return ret

    def get_siphash_keys(self):
        header_nonce = self.header.serialize()
        header_nonce += struct.pack("<Q", self.nonce)
        hash_header_nonce_as_str = sha256(header_nonce)
        key0 = struct.unpack("<Q", hash_header_nonce_as_str[0:8])[0]
        key1 = struct.unpack("<Q", hash_header_nonce_as_str[8:16])[0]
        return [ key0, key1 ]

    # Version 2 compact blocks use wtxid in shortids (rather than txid)
    def initialize_from_block(self, block, nonce=0, prefill_list = [0], use_witness = False):
        self.header = CBlockHeader(block)
        self.nonce = nonce
        self.prefilled_txn = [ PrefilledTransaction(i, block.vtx[i]) for i in prefill_list ]
        self.shortids = []
        self.use_witness = use_witness
        [k0, k1] = self.get_siphash_keys()
        for i in range(len(block.vtx)):
            if i not in prefill_list:
                tx_hash = block.vtx[i].sha256
                if use_witness:
                    tx_hash = block.vtx[i].calc_sha256(with_witness=True)
                self.shortids.append(calculate_shortid(k0, k1, tx_hash))

    def __repr__(self):
        return "HeaderAndShortIDs(header=%s, nonce=%d, shortids=%s, prefilledtxn=%s" % (repr(self.header), self.nonce, repr(self.shortids), repr(self.prefilled_txn))


class BlockTransactionsRequest(object):

    def __init__(self, blockhash=0, indexes = None):
        self.blockhash = blockhash
        self.indexes = indexes if indexes != None else []

    def deserialize(self, f):
        self.blockhash = deser_uint256(f)
        indexes_length = deser_compact_size(f)
        for i in range(indexes_length):
            self.indexes.append(deser_compact_size(f))

    def serialize(self):
        r = b""
        r += ser_uint256(self.blockhash)
        r += ser_compact_size(len(self.indexes))
        for x in self.indexes:
            r += ser_compact_size(x)
        return r

    # helper to set the differentially encoded indexes from absolute ones
    def from_absolute(self, absolute_indexes):
        self.indexes = []
        last_index = -1
        for x in absolute_indexes:
            self.indexes.append(x-last_index-1)
            last_index = x

    def to_absolute(self):
        absolute_indexes = []
        last_index = -1
        for x in self.indexes:
            absolute_indexes.append(x+last_index+1)
            last_index = absolute_indexes[-1]
        return absolute_indexes

    def __repr__(self):
        return "BlockTransactionsRequest(hash=%064x indexes=%s)" % (self.blockhash, repr(self.indexes))


class BlockTransactions(object):

    def __init__(self, blockhash=0, transactions = None):
        self.blockhash = blockhash
        self.transactions = transactions if transactions != None else []

    def deserialize(self, f):
        self.blockhash = deser_uint256(f)
        self.transactions = deser_vector(f, CTransaction)

    def serialize(self, with_witness=False):
        r = b""
        r += ser_uint256(self.blockhash)
        if with_witness:
            r += ser_vector(self.transactions, "serialize_with_witness")
        else:
            r += ser_vector(self.transactions)
        return r

    def __repr__(self):
        return "BlockTransactions(hash=%064x transactions=%s)" % (self.blockhash, repr(self.transactions))


# Objects that correspond to messages on the wire
class msg_version(object):
    command = b"version"

    def __init__(self):
        self.nVersion = MY_VERSION
        self.nServices = 1
        self.nTime = int(time.time())
        self.addrTo = CAddress()
        self.addrFrom = CAddress()
        self.nNonce = random.getrandbits(64)
        self.strSubVer = MY_SUBVERSION
        self.nStartingHeight = -1

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        if self.nVersion == 10300:
            self.nVersion = 300
        self.nServices = struct.unpack("<Q", f.read(8))[0]
        self.nTime = struct.unpack("<q", f.read(8))[0]
        self.addrTo = CAddress()
        self.addrTo.deserialize(f)
        if self.nVersion >= 106:
            self.addrFrom = CAddress()
            self.addrFrom.deserialize(f)
            self.nNonce = struct.unpack("<Q", f.read(8))[0]
            self.strSubVer = deser_string(f)
            if self.nVersion >= 209:
                self.nStartingHeight = struct.unpack("<i", f.read(4))[0]
            else:
                self.nStartingHeight = None
        else:
            self.addrFrom = None
            self.nNonce = None
            self.strSubVer = None
            self.nStartingHeight = None

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += struct.pack("<Q", self.nServices)
        r += struct.pack("<q", self.nTime)
        r += self.addrTo.serialize()
        r += self.addrFrom.serialize()
        r += struct.pack("<Q", self.nNonce)
        r += ser_string(self.strSubVer)
        r += struct.pack("<i", self.nStartingHeight)
        return r

    def __repr__(self):
        return 'msg_version(nVersion=%i nServices=%i nTime=%s addrTo=%s addrFrom=%s nNonce=0x%016X strSubVer=%s nStartingHeight=%i)' \
            % (self.nVersion, self.nServices, time.ctime(self.nTime),
               repr(self.addrTo), repr(self.addrFrom), self.nNonce,
               self.strSubVer, self.nStartingHeight)


class msg_verack(object):
    command = b"verack"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_verack()"


class msg_addr(object):
    command = b"addr"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress)

    def serialize(self):
        return ser_vector(self.addrs)

    def __repr__(self):
        return "msg_addr(addrs=%s)" % (repr(self.addrs))


class msg_alert(object):
    command = b"alert"

    def __init__(self):
        self.alert = CAlert()

    def deserialize(self, f):
        self.alert = CAlert()
        self.alert.deserialize(f)

    def serialize(self):
        r = b""
        r += self.alert.serialize()
        return r

    def __repr__(self):
        return "msg_alert(alert=%s)" % (repr(self.alert), )


class msg_inv(object):
    command = b"inv"

    def __init__(self, inv=None):
        if inv is None:
            self.inv = []
        else:
            self.inv = inv

    def deserialize(self, f):
        self.inv = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.inv)

    def __repr__(self):
        return "msg_inv(inv=%s)" % (repr(self.inv))


class msg_getdata(object):
    command = b"getdata"

    def __init__(self, inv=None):
        self.inv = inv if inv != None else []

    def deserialize(self, f):
        self.inv = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.inv)

    def __repr__(self):
        return "msg_getdata(inv=%s)" % (repr(self.inv))


class msg_getblocks(object):
    command = b"getblocks"

    def __init__(self):
        self.locator = CBlockLocator()
        self.hashstop = 0

    def deserialize(self, f):
        self.locator = CBlockLocator()
        self.locator.deserialize(f)
        self.hashstop = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.locator.serialize()
        r += ser_uint256(self.hashstop)
        return r

    def __repr__(self):
        return "msg_getblocks(locator=%s hashstop=%064x)" \
            % (repr(self.locator), self.hashstop)


class msg_tx(object):
    command = b"tx"

    def __init__(self, tx=CTransaction()):
        self.tx = tx

    def deserialize(self, f):
        self.tx.deserialize(f)

    def serialize(self):
        return self.tx.serialize_without_witness()

    def __repr__(self):
        return "msg_tx(tx=%s)" % (repr(self.tx))

class msg_witness_tx(msg_tx):

    def serialize(self):
        return self.tx.serialize_with_witness()


class msg_block(object):
    command = b"block"

    def __init__(self, block=None):
        if block is None:
            self.block = CBlock()
        else:
            self.block = block

    def deserialize(self, f):
        self.block.deserialize(f)

    def serialize(self):
        return self.block.serialize()

    def __repr__(self):
        return "msg_block(block=%s)" % (repr(self.block))

# for cases where a user needs tighter control over what is sent over the wire
# note that the user must supply the name of the command, and the data
class msg_generic(object):
    def __init__(self, command, data=None):
        self.command = command
        self.data = data

    def serialize(self):
        return self.data

    def __repr__(self):
        return "msg_generic()"

class msg_witness_block(msg_block):

    def serialize(self):
        r = self.block.serialize(with_witness=True)
        return r

class msg_getaddr(object):
    command = b"getaddr"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_getaddr()"


class msg_ping_prebip31(object):
    command = b"ping"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_ping() (pre-bip31)"


class msg_ping(object):
    command = b"ping"

    def __init__(self, nonce=0):
        self.nonce = nonce

    def deserialize(self, f):
        self.nonce = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.nonce)
        return r

    def __repr__(self):
        return "msg_ping(nonce=%08x)" % self.nonce


class msg_pong(object):
    command = b"pong"

    def __init__(self, nonce=0):
        self.nonce = nonce

    def deserialize(self, f):
        self.nonce = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.nonce)
        return r

    def __repr__(self):
        return "msg_pong(nonce=%08x)" % self.nonce


class msg_mempool(object):
    command = b"mempool"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_mempool()"

class msg_sendheaders(object):
    command = b"sendheaders"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_sendheaders()"


# getheaders message has
# number of entries
# vector of hashes
# hash_stop (hash of last desired block header, 0 to get as many as possible)
class msg_getheaders(object):
    command = b"getheaders"

    def __init__(self):
        self.locator = CBlockLocator()
        self.hashstop = 0

    def deserialize(self, f):
        self.locator = CBlockLocator()
        self.locator.deserialize(f)
        self.hashstop = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.locator.serialize()
        r += ser_uint256(self.hashstop)
        return r

    def __repr__(self):
        return "msg_getheaders(locator=%s, stop=%064x)" \
            % (repr(self.locator), self.hashstop)


# headers message has
# <count> <vector of block headers>
class msg_headers(object):
    command = b"headers"

    def __init__(self):
        self.headers = []

    def deserialize(self, f):
        # comment in freicoind indicates these should be deserialized as blocks
        blocks = deser_vector(f, CBlock)
        for x in blocks:
            self.headers.append(CBlockHeader(x))

    def serialize(self):
        blocks = [CBlock(x) for x in self.headers]
        return ser_vector(blocks)

    def __repr__(self):
        return "msg_headers(headers=%s)" % repr(self.headers)


class msg_reject(object):
    command = b"reject"
    REJECT_MALFORMED = 1

    def __init__(self):
        self.message = b""
        self.code = 0
        self.reason = b""
        self.data = 0

    def deserialize(self, f):
        self.message = deser_string(f)
        self.code = struct.unpack("<B", f.read(1))[0]
        self.reason = deser_string(f)
        if (self.code != self.REJECT_MALFORMED and
                (self.message == b"block" or self.message == b"tx")):
            self.data = deser_uint256(f)

    def serialize(self):
        r = ser_string(self.message)
        r += struct.pack("<B", self.code)
        r += ser_string(self.reason)
        if (self.code != self.REJECT_MALFORMED and
                (self.message == b"block" or self.message == b"tx")):
            r += ser_uint256(self.data)
        return r

    def __repr__(self):
        return "msg_reject: %s %d %s [%064x]" \
            % (self.message, self.code, self.reason, self.data)

# Helper function
def wait_until(predicate, *, attempts=float('inf'), timeout=float('inf')):
    attempt = 0
    elapsed = 0

    while attempt < attempts and elapsed < timeout:
        with mininode_lock:
            if predicate():
                return True
        attempt += 1
        elapsed += 0.05
        time.sleep(0.05)

    return False

class msg_feefilter(object):
    command = b"feefilter"

    def __init__(self, feerate=0):
        self.feerate = feerate

    def deserialize(self, f):
        self.feerate = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.feerate)
        return r

    def __repr__(self):
        return "msg_feefilter(feerate=%08x)" % self.feerate

class msg_sendcmpct(object):
    command = b"sendcmpct"

    def __init__(self):
        self.announce = False
        self.version = 1

    def deserialize(self, f):
        self.announce = struct.unpack("<?", f.read(1))[0]
        self.version = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<?", self.announce)
        r += struct.pack("<Q", self.version)
        return r

    def __repr__(self):
        return "msg_sendcmpct(announce=%s, version=%lu)" % (self.announce, self.version)

class msg_cmpctblock(object):
    command = b"cmpctblock"

    def __init__(self, header_and_shortids = None):
        self.header_and_shortids = header_and_shortids

    def deserialize(self, f):
        self.header_and_shortids = P2PHeaderAndShortIDs()
        self.header_and_shortids.deserialize(f)

    def serialize(self):
        r = b""
        r += self.header_and_shortids.serialize()
        return r

    def __repr__(self):
        return "msg_cmpctblock(HeaderAndShortIDs=%s)" % repr(self.header_and_shortids)

class msg_getblocktxn(object):
    command = b"getblocktxn"

    def __init__(self):
        self.block_txn_request = None

    def deserialize(self, f):
        self.block_txn_request = BlockTransactionsRequest()
        self.block_txn_request.deserialize(f)

    def serialize(self):
        r = b""
        r += self.block_txn_request.serialize()
        return r

    def __repr__(self):
        return "msg_getblocktxn(block_txn_request=%s)" % (repr(self.block_txn_request))

class msg_blocktxn(object):
    command = b"blocktxn"

    def __init__(self):
        self.block_transactions = BlockTransactions()

    def deserialize(self, f):
        self.block_transactions.deserialize(f)

    def serialize(self):
        r = b""
        r += self.block_transactions.serialize()
        return r

    def __repr__(self):
        return "msg_blocktxn(block_transactions=%s)" % (repr(self.block_transactions))

class msg_witness_blocktxn(msg_blocktxn):
    def serialize(self):
        r = b""
        r += self.block_transactions.serialize(with_witness=True)
        return r

# This is what a callback should look like for NodeConn
# Reimplement the on_* functions to provide handling for events
class NodeConnCB(object):
    def __init__(self):
        self.verack_received = False
        # deliver_sleep_time is helpful for debugging race conditions in p2p
        # tests; it causes message delivery to sleep for the specified time
        # before acquiring the global lock and delivering the next message.
        self.deliver_sleep_time = None
        # Remember the services our peer has advertised
        self.peer_services = None

    def set_deliver_sleep_time(self, value):
        with mininode_lock:
            self.deliver_sleep_time = value

    def get_deliver_sleep_time(self):
        with mininode_lock:
            return self.deliver_sleep_time

    # Spin until verack message is received from the node.
    # Tests may want to use this as a signal that the test can begin.
    # This can be called from the testing thread, so it needs to acquire the
    # global lock.
    def wait_for_verack(self):
        while True:
            with mininode_lock:
                if self.verack_received:
                    return
            time.sleep(0.05)

    def deliver(self, conn, message):
        deliver_sleep = self.get_deliver_sleep_time()
        if deliver_sleep is not None:
            time.sleep(deliver_sleep)
        with mininode_lock:
            try:
                getattr(self, 'on_' + message.command.decode('ascii'))(conn, message)
            except:
                print("ERROR delivering %s (%s)" % (repr(message),
                                                    sys.exc_info()[0]))

    def on_version(self, conn, message):
        if message.nVersion >= 209:
            conn.send_message(msg_verack())
        conn.ver_send = min(MY_VERSION, message.nVersion)
        if message.nVersion < 209:
            conn.ver_recv = conn.ver_send
        conn.nServices = message.nServices

    def on_verack(self, conn, message):
        conn.ver_recv = conn.ver_send
        self.verack_received = True

    def on_inv(self, conn, message):
        want = msg_getdata()
        for i in message.inv:
            if i.type != 0:
                want.inv.append(i)
        if len(want.inv):
            conn.send_message(want)

    def on_addr(self, conn, message): pass
    def on_alert(self, conn, message): pass
    def on_getdata(self, conn, message): pass
    def on_getblocks(self, conn, message): pass
    def on_tx(self, conn, message): pass
    def on_block(self, conn, message): pass
    def on_getaddr(self, conn, message): pass
    def on_headers(self, conn, message): pass
    def on_getheaders(self, conn, message): pass
    def on_ping(self, conn, message):
        if conn.ver_send > BIP0031_VERSION:
            conn.send_message(msg_pong(message.nonce))
    def on_reject(self, conn, message): pass
    def on_close(self, conn): pass
    def on_mempool(self, conn): pass
    def on_pong(self, conn, message): pass
    def on_feefilter(self, conn, message): pass
    def on_sendheaders(self, conn, message): pass
    def on_sendcmpct(self, conn, message): pass
    def on_cmpctblock(self, conn, message): pass
    def on_getblocktxn(self, conn, message): pass
    def on_blocktxn(self, conn, message): pass

# More useful callbacks and functions for NodeConnCB's which have a single NodeConn
class SingleNodeConnCB(NodeConnCB):
    def __init__(self):
        NodeConnCB.__init__(self)
        self.connection = None
        self.ping_counter = 1
        self.last_pong = msg_pong()

    def add_connection(self, conn):
        self.connection = conn

    # Wrapper for the NodeConn's send_message function
    def send_message(self, message):
        self.connection.send_message(message)

    def send_and_ping(self, message):
        self.send_message(message)
        self.sync_with_ping()

    def on_pong(self, conn, message):
        self.last_pong = message

    # Sync up with the node
    def sync_with_ping(self, timeout=30):
        def received_pong():
            return (self.last_pong.nonce == self.ping_counter)
        self.send_message(msg_ping(nonce=self.ping_counter))
        success = wait_until(received_pong, timeout=timeout)
        self.ping_counter += 1
        return success

# The actual NodeConn class
# This class provides an interface for a p2p connection to a specified node
class NodeConn(asyncore.dispatcher):
    messagemap = {
        b"version": msg_version,
        b"verack": msg_verack,
        b"addr": msg_addr,
        b"alert": msg_alert,
        b"inv": msg_inv,
        b"getdata": msg_getdata,
        b"getblocks": msg_getblocks,
        b"tx": msg_tx,
        b"block": msg_block,
        b"getaddr": msg_getaddr,
        b"ping": msg_ping,
        b"pong": msg_pong,
        b"headers": msg_headers,
        b"getheaders": msg_getheaders,
        b"reject": msg_reject,
        b"mempool": msg_mempool,
        b"feefilter": msg_feefilter,
        b"sendheaders": msg_sendheaders,
        b"sendcmpct": msg_sendcmpct,
        b"cmpctblock": msg_cmpctblock,
        b"getblocktxn": msg_getblocktxn,
        b"blocktxn": msg_blocktxn
    }
    MAGIC_BYTES = {
        "mainnet": b"\x2c\xfe\x7e\x6d",   # mainnet
        "testnet": b"\x5e\xd6\x7c\xf3",   # testnet
        "regtest": b"\xed\x99\x9c\xf6",   # regtest
    }

    def __init__(self, dstaddr, dstport, rpc, callback, net="regtest", services=NODE_NETWORK):
        asyncore.dispatcher.__init__(self, map=mininode_socket_map)
        self.log = logging.getLogger("NodeConn(%s:%d)" % (dstaddr, dstport))
        self.dstaddr = dstaddr
        self.dstport = dstport
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sendbuf = b""
        self.recvbuf = b""
        self.ver_send = 209
        self.ver_recv = 209
        self.last_sent = 0
        self.state = "connecting"
        self.network = net
        self.cb = callback
        self.disconnect = False
        self.nServices = 0

        # stuff version msg into sendbuf
        vt = msg_version()
        vt.nServices = services
        vt.addrTo.ip = self.dstaddr
        vt.addrTo.port = self.dstport
        vt.addrFrom.ip = "0.0.0.0"
        vt.addrFrom.port = 0
        self.send_message(vt, True)
        print('MiniNode: Connecting to Freicoin Node IP # ' + dstaddr + ':' \
            + str(dstport))

        try:
            self.connect((dstaddr, dstport))
        except:
            self.handle_close()
        self.rpc = rpc

    def show_debug_msg(self, msg):
        self.log.debug(msg)

    def handle_connect(self):
        self.show_debug_msg("MiniNode: Connected & Listening: \n")
        self.state = "connected"

    def handle_close(self):
        self.show_debug_msg("MiniNode: Closing Connection to %s:%d... "
                            % (self.dstaddr, self.dstport))
        self.state = "closed"
        self.recvbuf = b""
        self.sendbuf = b""
        try:
            self.close()
        except:
            pass
        self.cb.on_close(self)

    def handle_read(self):
        try:
            t = self.recv(8192)
            if len(t) > 0:
                self.recvbuf += t
                self.got_data()
        except:
            pass

    def readable(self):
        return True

    def writable(self):
        with mininode_lock:
            length = len(self.sendbuf)
        return (length > 0)

    def handle_write(self):
        with mininode_lock:
            try:
                sent = self.send(self.sendbuf)
            except:
                self.handle_close()
                return
            self.sendbuf = self.sendbuf[sent:]

    def got_data(self):
        try:
            while True:
                if len(self.recvbuf) < 4:
                    return
                if self.recvbuf[:4] != self.MAGIC_BYTES[self.network]:
                    raise ValueError("got garbage %s" % repr(self.recvbuf))
                if self.ver_recv < 209:
                    if len(self.recvbuf) < 4 + 12 + 4:
                        return
                    command = self.recvbuf[4:4+12].split(b"\x00", 1)[0]
                    msglen = struct.unpack("<i", self.recvbuf[4+12:4+12+4])[0]
                    checksum = None
                    if len(self.recvbuf) < 4 + 12 + 4 + msglen:
                        return
                    msg = self.recvbuf[4+12+4:4+12+4+msglen]
                    self.recvbuf = self.recvbuf[4+12+4+msglen:]
                else:
                    if len(self.recvbuf) < 4 + 12 + 4 + 4:
                        return
                    command = self.recvbuf[4:4+12].split(b"\x00", 1)[0]
                    msglen = struct.unpack("<i", self.recvbuf[4+12:4+12+4])[0]
                    checksum = self.recvbuf[4+12+4:4+12+4+4]
                    if len(self.recvbuf) < 4 + 12 + 4 + 4 + msglen:
                        return
                    msg = self.recvbuf[4+12+4+4:4+12+4+4+msglen]
                    th = sha256(msg)
                    h = sha256(th)
                    if checksum != h[:4]:
                        raise ValueError("got bad checksum " + repr(self.recvbuf))
                    self.recvbuf = self.recvbuf[4+12+4+4+msglen:]
                if command in self.messagemap:
                    f = BytesIO(msg)
                    t = self.messagemap[command]()
                    t.deserialize(f)
                    self.got_message(t)
                else:
                    self.show_debug_msg("Unknown command: '" + command + "' " +
                                        repr(msg))
        except Exception as e:
            print('got_data:', repr(e))
            # import  traceback
            # traceback.print_tb(sys.exc_info()[2])

    def send_message(self, message, pushbuf=False):
        if self.state != "connected" and not pushbuf:
            raise IOError('Not connected, no pushbuf')
        self.show_debug_msg("Send %s" % repr(message))
        command = message.command
        data = message.serialize()
        tmsg = self.MAGIC_BYTES[self.network]
        tmsg += command
        tmsg += b"\x00" * (12 - len(command))
        tmsg += struct.pack("<I", len(data))
        if self.ver_send >= 209:
            th = sha256(data)
            h = sha256(th)
            tmsg += h[:4]
        tmsg += data
        with mininode_lock:
            self.sendbuf += tmsg
            self.last_sent = time.time()

    def got_message(self, message):
        if message.command == b"version":
            if message.nVersion <= BIP0031_VERSION:
                self.messagemap[b'ping'] = msg_ping_prebip31
        if self.last_sent + 30 * 60 < time.time():
            self.send_message(self.messagemap[b'ping']())
        self.show_debug_msg("Recv %s" % repr(message))
        self.cb.deliver(self, message)

    def disconnect_node(self):
        self.disconnect = True


class NetworkThread(Thread):
    def run(self):
        while mininode_socket_map:
            # We check for whether to disconnect outside of the asyncore
            # loop to workaround the behavior of asyncore when using
            # select
            disconnected = []
            for fd, obj in mininode_socket_map.items():
                if obj.disconnect:
                    disconnected.append(obj)
            [ obj.handle_close() for obj in disconnected ]
            asyncore.loop(0.1, use_poll=True, map=mininode_socket_map, count=1)


# An exception we can raise if we detect a potential disconnect
# (p2p or rpc) before the test is complete
class EarlyDisconnectError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)
