#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-2022 The Bitcoin Core developers
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
"""Freicoin test framework primitive and message structures

CBlock, CTransaction, CBlockHeader, CTxIn, CTxOut, etc....:
    data structures that should map to corresponding structures in
    freicoin/primitives

msg_block, msg_tx, msg_headers, etc.:
    data structures that represent network messages

ser_*, deser_*: functions that handle serialization/deserialization.

Classes use __slots__ to ensure extraneous attributes aren't accidentally added
by tests, compromising their intended effect.
"""
from base64 import b32decode, b32encode
import copy
import hashlib
from io import BytesIO
import math
import random
import socket
import time
import unittest

from test_framework.crypto.siphash import siphash256
from test_framework.util import assert_equal

MAX_LOCATOR_SZ = 101
MAX_BLOCK_WEIGHT = 4000000
MAX_BLOOM_FILTER_SIZE = 36000
MAX_BLOOM_HASH_FUNCS = 50

COIN = 100000000  # 1 frc in kria
MAX_MONEY = 9007199254740991

MAX_BIP125_RBF_SEQUENCE = 0xfffffffd  # Sequence number that is rbf-opt-in (BIP 125) and csv-opt-out (BIP 68)
SEQUENCE_FINAL = 0xffffffff  # Sequence number that disables nLockTime if set for every input of a tx

MAX_PROTOCOL_MESSAGE_LENGTH = 4000000  # Maximum length of incoming protocol messages
MAX_HEADERS_RESULTS = 2000  # Number of headers sent in one getheaders result
MAX_INV_SIZE = 50000  # Maximum number of entries in an 'inv' protocol message

NODE_NETWORK = (1 << 0)
NODE_BLOOM = (1 << 2)
NODE_WITNESS = (1 << 3)
NODE_COMPACT_FILTERS = (1 << 6)
NODE_NETWORK_LIMITED = (1 << 10)
NODE_P2P_V2 = (1 << 11)

MSG_TX = 1
MSG_BLOCK = 2
MSG_FILTERED_BLOCK = 3
MSG_CMPCT_BLOCK = 4
MSG_WTX = 5
MSG_WITNESS_FLAG = 1 << 30
MSG_TYPE_MASK = 0xffffffff >> 2
MSG_WITNESS_TX = MSG_TX | MSG_WITNESS_FLAG

FILTER_TYPE_BASIC = 0

WITNESS_SCALE_FACTOR = 4

DEFAULT_ANCESTOR_LIMIT = 25    # default max number of in-mempool ancestors
DEFAULT_DESCENDANT_LIMIT = 25  # default max number of in-mempool descendants

# Default setting for -datacarriersize. 80 bytes of data, +1 for OP_RETURN, +2 for the pushdata opcodes.
MAX_OP_RETURN_RELAY = 83

DEFAULT_MEMPOOL_EXPIRY_HOURS = 336  # hours

MAGIC_BYTES = {
    "mainnet": b"\x2c\xfe\x7e\x6d",   # mainnet
    "testnet": b"\x5e\xd6\x7c\xf3",   # testnet
    "regtest": b"\xed\x99\x9c\xf6",   # regtest
    "signet": b"\xbc\x04\xa7\x36",    # signet
}

def sha256(s):
    return hashlib.sha256(s).digest()


def sha3(s):
    return hashlib.sha3_256(s).digest()


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
        r = l.to_bytes(1, "little")
    elif l < 0x10000:
        r = (253).to_bytes(1, "little") + l.to_bytes(2, "little")
    elif l < 0x100000000:
        r = (254).to_bytes(1, "little") + l.to_bytes(4, "little")
    else:
        r = (255).to_bytes(1, "little") + l.to_bytes(8, "little")
    return r


def deser_compact_size(f):
    nit = int.from_bytes(f.read(1), "little")
    if nit == 253:
        nit = int.from_bytes(f.read(2), "little")
    elif nit == 254:
        nit = int.from_bytes(f.read(4), "little")
    elif nit == 255:
        nit = int.from_bytes(f.read(8), "little")
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
    return int.from_bytes(f.read(32), 'little')


def ser_uint256(u):
    return u.to_bytes(32, 'little')


def uint256_from_str(s):
    return int.from_bytes(s[:32], 'little')


def uint256_from_compact(c):
    nbytes = (c >> 24) & 0xFF
    v = (c & 0xFFFFFF) << (8 * (nbytes - 3))
    return v


# deser_function_name: Allow for an alternate deserialization function on the
# entries in the vector.
def deser_vector(f, c, deser_function_name=None):
    nit = deser_compact_size(f)
    r = []
    for _ in range(nit):
        t = c()
        if deser_function_name:
            getattr(t, deser_function_name)(f)
        else:
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
    for _ in range(nit):
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
    for _ in range(nit):
        t = deser_string(f)
        r.append(t)
    return r


def ser_string_vector(l):
    r = ser_compact_size(len(l))
    for sv in l:
        r += ser_string(sv)
    return r


def from_hex(obj, hex_string):
    """Deserialize from a hex string representation (e.g. from RPC)

    Note that there is no complementary helper like e.g. `to_hex` for the
    inverse operation. To serialize a message object to a hex string, simply
    use obj.serialize().hex()"""
    obj.deserialize(BytesIO(bytes.fromhex(hex_string)))
    return obj


def tx_from_hex(hex_string):
    """Deserialize from hex string to a transaction object"""
    return from_hex(CTransaction(), hex_string)


# like from_hex, but without the hex part
def from_binary(cls, stream):
    """deserialize a binary stream (or bytes object) into an object"""
    # handle bytes object by turning it into a stream
    was_bytes = isinstance(stream, bytes)
    if was_bytes:
        stream = BytesIO(stream)
    obj = cls()
    obj.deserialize(stream)
    if was_bytes:
        assert len(stream.read()) == 0
    return obj


# Objects that map to freicoind objects, which can be serialized/deserialized


class CAddress:
    __slots__ = ("net", "ip", "nServices", "port", "time")

    # see https://github.com/bitcoin/bips/blob/master/bip-0155.mediawiki
    NET_IPV4 = 1
    NET_IPV6 = 2
    NET_TORV3 = 4
    NET_I2P = 5
    NET_CJDNS = 6

    ADDRV2_NET_NAME = {
        NET_IPV4: "IPv4",
        NET_IPV6: "IPv6",
        NET_TORV3: "TorV3",
        NET_I2P: "I2P",
        NET_CJDNS: "CJDNS"
    }

    ADDRV2_ADDRESS_LENGTH = {
        NET_IPV4: 4,
        NET_IPV6: 16,
        NET_TORV3: 32,
        NET_I2P: 32,
        NET_CJDNS: 16
    }

    I2P_PAD = "===="

    def __init__(self):
        self.time = 0
        self.nServices = 1
        self.net = self.NET_IPV4
        self.ip = "0.0.0.0"
        self.port = 0

    def __eq__(self, other):
        return self.net == other.net and self.ip == other.ip and self.nServices == other.nServices and self.port == other.port and self.time == other.time

    def deserialize(self, f, *, with_time=True):
        """Deserialize from addrv1 format (pre-BIP155)"""
        if with_time:
            # VERSION messages serialize CAddress objects without time
            self.time = int.from_bytes(f.read(4), "little")
        self.nServices = int.from_bytes(f.read(8), "little")
        # We only support IPv4 which means skip 12 bytes and read the next 4 as IPv4 address.
        f.read(12)
        self.net = self.NET_IPV4
        self.ip = socket.inet_ntoa(f.read(4))
        self.port = int.from_bytes(f.read(2), "big")

    def serialize(self, *, with_time=True):
        """Serialize in addrv1 format (pre-BIP155)"""
        assert self.net == self.NET_IPV4
        r = b""
        if with_time:
            # VERSION messages serialize CAddress objects without time
            r += self.time.to_bytes(4, "little")
        r += self.nServices.to_bytes(8, "little")
        r += b"\x00" * 10 + b"\xff" * 2
        r += socket.inet_aton(self.ip)
        r += self.port.to_bytes(2, "big")
        return r

    def deserialize_v2(self, f):
        """Deserialize from addrv2 format (BIP155)"""
        self.time = int.from_bytes(f.read(4), "little")

        self.nServices = deser_compact_size(f)

        self.net = int.from_bytes(f.read(1), "little")
        assert self.net in self.ADDRV2_NET_NAME

        address_length = deser_compact_size(f)
        assert address_length == self.ADDRV2_ADDRESS_LENGTH[self.net]

        addr_bytes = f.read(address_length)
        if self.net == self.NET_IPV4:
            self.ip = socket.inet_ntoa(addr_bytes)
        elif self.net == self.NET_IPV6:
            self.ip = socket.inet_ntop(socket.AF_INET6, addr_bytes)
        elif self.net == self.NET_TORV3:
            prefix = b".onion checksum"
            version = bytes([3])
            checksum = sha3(prefix + addr_bytes + version)[:2]
            self.ip = b32encode(addr_bytes + checksum + version).decode("ascii").lower() + ".onion"
        elif self.net == self.NET_I2P:
            self.ip = b32encode(addr_bytes)[0:-len(self.I2P_PAD)].decode("ascii").lower() + ".b32.i2p"
        elif self.net == self.NET_CJDNS:
            self.ip = socket.inet_ntop(socket.AF_INET6, addr_bytes)
        else:
            raise Exception(f"Address type not supported")

        self.port = int.from_bytes(f.read(2), "big")

    def serialize_v2(self):
        """Serialize in addrv2 format (BIP155)"""
        assert self.net in self.ADDRV2_NET_NAME
        r = b""
        r += self.time.to_bytes(4, "little")
        r += ser_compact_size(self.nServices)
        r += self.net.to_bytes(1, "little")
        r += ser_compact_size(self.ADDRV2_ADDRESS_LENGTH[self.net])
        if self.net == self.NET_IPV4:
            r += socket.inet_aton(self.ip)
        elif self.net == self.NET_IPV6:
            r += socket.inet_pton(socket.AF_INET6, self.ip)
        elif self.net == self.NET_TORV3:
            sfx = ".onion"
            assert self.ip.endswith(sfx)
            r += b32decode(self.ip[0:-len(sfx)], True)[0:32]
        elif self.net == self.NET_I2P:
            sfx = ".b32.i2p"
            assert self.ip.endswith(sfx)
            r += b32decode(self.ip[0:-len(sfx)] + self.I2P_PAD, True)
        elif self.net == self.NET_CJDNS:
            r += socket.inet_pton(socket.AF_INET6, self.ip)
        else:
            raise Exception(f"Address type not supported")
        r += self.port.to_bytes(2, "big")
        return r

    def __repr__(self):
        return ("CAddress(nServices=%i net=%s addr=%s port=%i)"
                % (self.nServices, self.ADDRV2_NET_NAME[self.net], self.ip, self.port))


class CInv:
    __slots__ = ("hash", "type")

    typemap = {
        0: "Error",
        MSG_TX: "TX",
        MSG_BLOCK: "Block",
        MSG_TX | MSG_WITNESS_FLAG: "WitnessTx",
        MSG_BLOCK | MSG_WITNESS_FLAG: "WitnessBlock",
        MSG_FILTERED_BLOCK: "filtered Block",
        MSG_CMPCT_BLOCK: "CompactBlock",
        MSG_WTX: "WTX",
    }

    def __init__(self, t=0, h=0):
        self.type = t
        self.hash = h

    def deserialize(self, f):
        self.type = int.from_bytes(f.read(4), "little")
        self.hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.type.to_bytes(4, "little")
        r += ser_uint256(self.hash)
        return r

    def __repr__(self):
        return "CInv(type=%s hash=%064x)" \
            % (self.typemap[self.type], self.hash)

    def __eq__(self, other):
        return isinstance(other, CInv) and self.hash == other.hash and self.type == other.type


class CBlockLocator:
    __slots__ = ("nVersion", "vHave")

    def __init__(self):
        self.vHave = []

    def deserialize(self, f):
        int.from_bytes(f.read(4), "little", signed=True)  # Ignore version field.
        self.vHave = deser_uint256_vector(f)

    def serialize(self):
        r = b""
        r += (0).to_bytes(4, "little", signed=True)  # Freicoin ignores the version field. Set it to 0.
        r += ser_uint256_vector(self.vHave)
        return r

    def __repr__(self):
        return "CBlockLocator(vHave=%s)" % (repr(self.vHave))


class COutPoint:
    __slots__ = ("hash", "n")

    def __init__(self, hash=0, n=0):
        self.hash = hash
        self.n = n

    def deserialize(self, f):
        self.hash = deser_uint256(f)
        self.n = int.from_bytes(f.read(4), "little")

    def serialize(self):
        r = b""
        r += ser_uint256(self.hash)
        r += self.n.to_bytes(4, "little")
        return r

    def __repr__(self):
        return "COutPoint(hash=%064x n=%i)" % (self.hash, self.n)


class CTxIn:
    __slots__ = ("nSequence", "prevout", "scriptSig")

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
        self.nSequence = int.from_bytes(f.read(4), "little")

    def serialize(self):
        r = b""
        r += self.prevout.serialize()
        r += ser_string(self.scriptSig)
        r += self.nSequence.to_bytes(4, "little")
        return r

    def __repr__(self):
        return "CTxIn(prevout=%s scriptSig=%s nSequence=%i)" \
            % (repr(self.prevout), self.scriptSig.hex(),
               self.nSequence)


class CTxOut:
    __slots__ = ("nValue", "scriptPubKey")

    def __init__(self, nValue=0, scriptPubKey=b""):
        self.nValue = nValue
        self.scriptPubKey = scriptPubKey

    def deserialize(self, f):
        self.nValue = int.from_bytes(f.read(8), "little", signed=True)
        self.scriptPubKey = deser_string(f)

    def serialize(self):
        r = b""
        r += self.nValue.to_bytes(8, "little", signed=True)
        r += ser_string(self.scriptPubKey)
        return r

    def __repr__(self):
        return "CTxOut(nValue=%i.%08i scriptPubKey=%s)" \
            % (self.nValue // COIN, self.nValue % COIN,
               self.scriptPubKey.hex())


class CScriptWitness:
    __slots__ = ("stack",)

    def __init__(self):
        # stack is a vector of strings
        self.stack = []

    def __repr__(self):
        return "CScriptWitness(%s)" % \
               (",".join([x.hex() for x in self.stack]))

    def is_null(self):
        if self.stack:
            return False
        return True


class CTxInWitness:
    __slots__ = ("scriptWitness",)

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


class CTxWitness:
    __slots__ = ("vtxinwit",)

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


class CTransaction:
    __slots__ = ("hash", "lock_height", "nLockTime", "nVersion", "sha256",
                 "vin", "vout", "wit")

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
        self.nVersion = int.from_bytes(f.read(4), "little", signed=True)
        flags = 0
        dummy = int.from_bytes(f.read(1), "little")
        if dummy == 255:
            flags = int.from_bytes(f.read(1), "little")
            self.vin = deser_vector(f, CTxIn)
        else:
            if dummy <= 252:
                size = dummy
            if dummy == 253:
                size = int.from_bytes(f.read(2), "little")
            if dummy == 254:
                size = int.from_bytes(f.read(4), "little")
            self.vin = []
            for _ in range(size):
                txin = CTxIn()
                txin.deserialize(f)
                self.vin.append(txin)
        self.vout = deser_vector(f, CTxOut)
        if flags != 0:
            self.wit.vtxinwit = [CTxInWitness() for _ in range(len(self.vin))]
            self.wit.deserialize(f)
        else:
            self.wit = CTxWitness()
        self.nLockTime = int.from_bytes(f.read(4), "little")
        if self.nVersion!=1 or len(self.vin)!=1 or self.vin[0].prevout.hash!=0 or self.vin[0].prevout.n not in (-1,0xffffffff):
            self.lock_height = int.from_bytes(f.read(4), "little")
        self.sha256 = None
        self.hash = None

    def serialize_without_witness(self):
        r = b""
        r += self.nVersion.to_bytes(4, "little", signed=True)
        r += ser_vector(self.vin)
        r += ser_vector(self.vout)
        r += self.nLockTime.to_bytes(4, "little")
        if self.nVersion!=1 or len(self.vin)!=1 or self.vin[0].prevout.hash!=0 or self.vin[0].prevout.n not in (-1,0xffffffff):
            r += self.lock_height.to_bytes(4, "little")
        return r

    # Only serialize with witness when explicitly called for
    def serialize_with_witness(self):
        flags = 0
        if not self.wit.is_null():
            flags |= 1
        r = b""
        r += self.nVersion.to_bytes(4, "little", signed=True)
        if flags:
            dummy = 255
            r += dummy.to_bytes(1, "little")
            r += flags.to_bytes(1, "little")
        r += ser_vector(self.vin)
        r += ser_vector(self.vout)
        if flags & 1:
            if (len(self.wit.vtxinwit) != len(self.vin)):
                # vtxinwit must have the same length as vin
                self.wit.vtxinwit = self.wit.vtxinwit[:len(self.vin)]
                for _ in range(len(self.wit.vtxinwit), len(self.vin)):
                    self.wit.vtxinwit.append(CTxInWitness())
            r += self.wit.serialize()
        r += self.nLockTime.to_bytes(4, "little")
        if self.nVersion!=1 or len(self.vin)!=1 or self.vin[0].prevout.hash!=0 or self.vin[0].prevout.n not in (-1,0xffffffff):
            r += self.lock_height.to_bytes(4, "little")
        return r

    # Regular serialization is with witness -- must explicitly
    # call serialize_without_witness to exclude witness data.
    def serialize(self):
        return self.serialize_with_witness()

    def getwtxid(self):
        return hash256(self.serialize())[::-1].hex()

    # Recalculate the txid (transaction hash without witness)
    def rehash(self):
        self.sha256 = None
        self.calc_sha256()
        return self.hash

    # We will only cache the serialization without witness in
    # self.sha256 and self.hash -- those are expected to be the txid.
    def calc_sha256(self, with_witness=False):
        if with_witness:
            # Don't cache the result, just return it
            return uint256_from_str(hash256(self.serialize_with_witness()))

        if self.sha256 is None:
            self.sha256 = uint256_from_str(hash256(self.serialize_without_witness()))
        self.hash = hash256(self.serialize_without_witness())[::-1].hex()

    def is_valid(self):
        self.calc_sha256()
        for tout in self.vout:
            if tout.nValue < 0 or tout.nValue > MAX_MONEY:
                return False
        return True

    # Calculate the transaction weight using witness and non-witness
    # serialization size (does NOT use sigops).
    def get_weight(self):
        with_witness_size = len(self.serialize_with_witness())
        without_witness_size = len(self.serialize_without_witness())
        return (WITNESS_SCALE_FACTOR - 1) * without_witness_size + with_witness_size

    def get_vsize(self):
        return math.ceil(self.get_weight() / WITNESS_SCALE_FACTOR)

    def __repr__(self):
        return "CTransaction(nVersion=%i vin=%s vout=%s wit=%s nLockTime=%i lock_height=%i)" \
            % (self.nVersion, repr(self.vin), repr(self.vout), repr(self.wit), self.nLockTime, self.lock_height)


class AuxProofOfWork(object):
    __slots__ = ("commit_version", "commit_hash_merkle_root", "commit_time", "commit_bits",
                 "commit_nonce", "secret_lo", "secret_hi", "midstate_hash", "midstate_buffer",
                 "midstate_length", "aux_lock_time", "aux_branch", "aux_num_txns", "aux_version",
                 "aux_hash_prev_block", "aux_bits", "aux_nonce")

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

class CBlockHeader:
    __slots__ = ("hash", "hashMerkleRoot", "hashPrevBlock", "nBits", "nNonce",
                 "nTime", "nVersion", "sha256", "aux_pow", "aux_hash1", "aux_hash2")

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
        self.nVersion = 4
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
        self.nVersion = int.from_bytes(f.read(4), "little", signed=True)
        self.hashPrevBlock = deser_uint256(f)
        self.hashMerkleRoot = deser_uint256(f)
        self.nTime = int.from_bytes(f.read(4), "little")
        self.nBits = int.from_bytes(f.read(4), "little")
        extended = False
        if self.nBits & 0x00800000:
            self.nBits = self.nBits ^ 0x00800000
            extended = True
        self.nNonce = int.from_bytes(f.read(4), "little")
        self.aux_pow = False
        if extended:
            dummy = int.from_bytes(f.read(1), "little")
            assert(dummy == 0xff)
            flags = int.from_bytes(f.read(1), "little")
            if flags & 1:
                flags = flags ^ 1
                self.aux_pow = AuxProofOfWork()
                self.aux_pow.deserialize(f)
            assert(flags == 0x00)
        self.sha256 = None
        self.hash = None

    def serialize(self):
        r = b""
        r += self.nVersion.to_bytes(4, "little", signed=True)
        r += ser_uint256(self.hashPrevBlock)
        r += ser_uint256(self.hashMerkleRoot)
        r += self.nTime.to_bytes(4, "little")
        bits = self.nBits
        if self.aux_pow:
            bits = bits | 0x00800000
        r += bits.to_bytes(4, "little")
        if self.aux_pow:
            r += (0xff).to_bytes(1, "little")
            r += (0x01).to_bytes(1, "little")
            r += self.aux_pow.serialize()
        r += self.nNonce.to_bytes(4, "little")
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
            r += self.nVersion.to_bytes(4, "little", signed=True)
            r += ser_uint256(self.hashPrevBlock)
            r += ser_uint256(self.hashMerkleRoot)
            r += self.nTime.to_bytes(4, "little")
            r += self.nBits.to_bytes(4, "little")
            r += self.nNonce.to_bytes(4, "little")
            self.sha256 = uint256_from_str(hash256(r))
            self.hash = hash256(r)[::-1].hex()

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

BLOCK_HEADER_SIZE = len(CBlockHeader().serialize())
assert_equal(BLOCK_HEADER_SIZE, 80)

class CBlock(CBlockHeader):
    __slots__ = ("vtx",)

    def __init__(self, header=None):
        super().__init__(header)
        self.vtx = []

    def deserialize(self, f):
        super().deserialize(f)
        self.vtx = deser_vector(f, CTransaction)

    def serialize(self, with_witness=True):
        r = b""
        r += super().serialize()
        if with_witness:
            r += ser_vector(self.vtx, "serialize_with_witness")
        else:
            r += ser_vector(self.vtx, "serialize_without_witness")
        return r

    def get_bias(self):
        return 0 if not self.aux_pow else (self.aux_pow.commit_nonce & 0xff)

    def set_bias(self, bias):
        if self.aux_pow:
            self.aux_pow.commit_nonce = (self.aux_pow.commit_nonce & 0xffffff00) | (bias & 0xff)

    # Calculate the merkle root given a vector of transaction hashes
    @classmethod
    def get_merkle_root(cls, hashes):
        while len(hashes) > 1:
            newhashes = []
            for i in range(0, len(hashes), 2):
                i2 = min(i+1, len(hashes)-1)
                newhashes.append(hash256(hashes[i] + hashes[i2]))
            hashes = newhashes
        return uint256_from_str(hashes[0])

    # Calculate the Merkle root using fast Merkle trees, given a
    # vector of transaction hashes.
    @classmethod
    def get_fast_merkle_root(cls, hashes):
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
            self.vtx[0].vin[0].scriptSig = bytes.fromhex(self.vtx[0].vin[0].scriptSig.hex()) + ser_uint256(self.aux_hash2)
            self.vtx[0].rehash()
            self.hashMerkleRoot = self.calc_merkle_root()
        self.rehash()
        target = uint256_from_compact(self.nBits)
        while self.sha256 > target:
            self.nNonce += 1
            self.rehash()

    # Calculate the block weight using witness and non-witness
    # serialization size (does NOT use sigops).
    def get_weight(self):
        offset = len(CBlockHeader(self).serialize()) - len(CBlockHeader().serialize())
        with_witness_size = len(self.serialize(with_witness=True)) - offset
        without_witness_size = len(self.serialize(with_witness=False)) - offset
        return (WITNESS_SCALE_FACTOR - 1) * without_witness_size + with_witness_size

    def get_virtual_size(witness_block):
        """Calculate the virtual size of a witness block.

        Virtual size is base + witness/4."""
        base_size = len(witness_block.serialize(with_witness=False))
        total_size = len(witness_block.serialize())
        # the "+3" is so we round up
        vsize = int((3 * base_size + total_size + 3) / 4)
        return vsize

    def __repr__(self):
        aux_pow_str = ""
        if self.aux_pow:
            aux_pow_str = " aux_pow=%s" % repr(self.aux_pow)
        return "CBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x%s vtx=%s)" \
            % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
               time.ctime(self.nTime), self.nBits, self.nNonce, aux_pow_str, repr(self.vtx))


class PrefilledTransaction:
    __slots__ = ("index", "tx")

    def __init__(self, index=0, tx = None):
        self.index = index
        self.tx = tx

    def deserialize(self, f):
        self.index = deser_compact_size(f)
        self.tx = CTransaction()
        self.tx.deserialize(f)

    def serialize(self, with_witness=True):
        r = b""
        r += ser_compact_size(self.index)
        if with_witness:
            r += self.tx.serialize_with_witness()
        else:
            r += self.tx.serialize_without_witness()
        return r

    def serialize_without_witness(self):
        return self.serialize(with_witness=False)

    def serialize_with_witness(self):
        return self.serialize(with_witness=True)

    def __repr__(self):
        return "PrefilledTransaction(index=%d, tx=%s)" % (self.index, repr(self.tx))


# This is what we send on the wire, in a cmpctblock message.
class P2PHeaderAndShortIDs:
    __slots__ = ("header", "nonce", "prefilled_txn", "prefilled_txn_length",
                 "shortids", "shortids_length")

    def __init__(self):
        self.header = CBlockHeader()
        self.nonce = 0
        self.shortids_length = 0
        self.shortids = []
        self.prefilled_txn_length = 0
        self.prefilled_txn = []

    def deserialize(self, f):
        self.header.deserialize(f)
        self.nonce = int.from_bytes(f.read(8), "little")
        self.shortids_length = deser_compact_size(f)
        for _ in range(self.shortids_length):
            # shortids are defined to be 6 bytes in the spec, so append
            # two zero bytes and read it in as an 8-byte number
            self.shortids.append(int.from_bytes(f.read(6) + b'\x00\x00', "little"))
        self.prefilled_txn = deser_vector(f, PrefilledTransaction)
        self.prefilled_txn_length = len(self.prefilled_txn)

    # When using version 2 compact blocks, we must serialize with_witness.
    def serialize(self, with_witness=False):
        r = b""
        r += self.header.serialize()
        r += self.nonce.to_bytes(8, "little")
        r += ser_compact_size(self.shortids_length)
        for x in self.shortids:
            # We only want the first 6 bytes
            r += x.to_bytes(8, "little")[0:6]
        if with_witness:
            r += ser_vector(self.prefilled_txn, "serialize_with_witness")
        else:
            r += ser_vector(self.prefilled_txn, "serialize_without_witness")
        return r

    def __repr__(self):
        return "P2PHeaderAndShortIDs(header=%s, nonce=%d, shortids_length=%d, shortids=%s, prefilled_txn_length=%d, prefilledtxn=%s" % (repr(self.header), self.nonce, self.shortids_length, repr(self.shortids), self.prefilled_txn_length, repr(self.prefilled_txn))


# P2P version of the above that will use witness serialization (for compact
# block version 2)
class P2PHeaderAndShortWitnessIDs(P2PHeaderAndShortIDs):
    __slots__ = ()
    def serialize(self):
        return super().serialize(with_witness=True)

# Calculate the BIP 152-compact blocks shortid for a given transaction hash
def calculate_shortid(k0, k1, tx_hash):
    expected_shortid = siphash256(k0, k1, tx_hash)
    expected_shortid &= 0x0000ffffffffffff
    return expected_shortid


# This version gets rid of the array lengths, and reinterprets the differential
# encoding into indices that can be used for lookup.
class HeaderAndShortIDs:
    __slots__ = ("header", "nonce", "prefilled_txn", "shortids", "use_witness")

    def __init__(self, p2pheaders_and_shortids = None):
        self.header = CBlockHeader()
        self.nonce = 0
        self.shortids = []
        self.prefilled_txn = []
        self.use_witness = False

        if p2pheaders_and_shortids is not None:
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
        header_nonce += self.nonce.to_bytes(8, "little")
        hash_header_nonce_as_str = sha256(header_nonce)
        key0 = int.from_bytes(hash_header_nonce_as_str[0:8], "little")
        key1 = int.from_bytes(hash_header_nonce_as_str[8:16], "little")
        return [ key0, key1 ]

    # Version 2 compact blocks use wtxid in shortids (rather than txid)
    def initialize_from_block(self, block, nonce=0, prefill_list=None, use_witness=False):
        if prefill_list is None:
            prefill_list = [0, len(block.vtx)-1]
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


class BlockTransactionsRequest:
    __slots__ = ("blockhash", "indexes")

    def __init__(self, blockhash=0, indexes = None):
        self.blockhash = blockhash
        self.indexes = indexes if indexes is not None else []

    def deserialize(self, f):
        self.blockhash = deser_uint256(f)
        indexes_length = deser_compact_size(f)
        for _ in range(indexes_length):
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


class BlockTransactions:
    __slots__ = ("blockhash", "transactions")

    def __init__(self, blockhash=0, transactions = None):
        self.blockhash = blockhash
        self.transactions = transactions if transactions is not None else []

    def deserialize(self, f):
        self.blockhash = deser_uint256(f)
        self.transactions = deser_vector(f, CTransaction)

    def serialize(self, with_witness=True):
        r = b""
        r += ser_uint256(self.blockhash)
        if with_witness:
            r += ser_vector(self.transactions, "serialize_with_witness")
        else:
            r += ser_vector(self.transactions, "serialize_without_witness")
        return r

    def __repr__(self):
        return "BlockTransactions(hash=%064x transactions=%s)" % (self.blockhash, repr(self.transactions))


class CPartialMerkleTree:
    __slots__ = ("nTransactions", "vBits", "vHash")

    def __init__(self):
        self.nTransactions = 0
        self.vHash = []
        self.vBits = []

    def deserialize(self, f):
        self.nTransactions = int.from_bytes(f.read(4), "little")
        self.vHash = deser_uint256_vector(f)
        vBytes = deser_string(f)
        self.vBits = []
        for i in range(len(vBytes) * 8):
            self.vBits.append(vBytes[i//8] & (1 << (i % 8)) != 0)

    def serialize(self):
        r = b""
        r += self.nTransactions.to_bytes(4, "little")
        r += ser_uint256_vector(self.vHash)
        vBytesArray = bytearray([0x00] * ((len(self.vBits) + 7)//8))
        for i in range(len(self.vBits)):
            vBytesArray[i // 8] |= self.vBits[i] << (i % 8)
        r += ser_string(bytes(vBytesArray))
        return r

    def __repr__(self):
        return "CPartialMerkleTree(nTransactions=%d, vHash=%s, vBits=%s)" % (self.nTransactions, repr(self.vHash), repr(self.vBits))


class CMerkleBlock:
    __slots__ = ("header", "txn")

    def __init__(self):
        self.header = CBlockHeader()
        self.txn = CPartialMerkleTree()

    def deserialize(self, f):
        self.header.deserialize(f)
        self.txn.deserialize(f)

    def serialize(self):
        r = b""
        r += self.header.serialize()
        r += self.txn.serialize()
        return r

    def __repr__(self):
        return "CMerkleBlock(header=%s, txn=%s)" % (repr(self.header), repr(self.txn))


# Objects that correspond to messages on the wire
class msg_version:
    __slots__ = ("addrFrom", "addrTo", "nNonce", "relay", "nServices",
                 "nStartingHeight", "nTime", "nVersion", "strSubVer")
    msgtype = b"version"

    def __init__(self):
        self.nVersion = 0
        self.nServices = 0
        self.nTime = int(time.time())
        self.addrTo = CAddress()
        self.addrFrom = CAddress()
        self.nNonce = random.getrandbits(64)
        self.strSubVer = ''
        self.nStartingHeight = -1
        self.relay = 0

    def deserialize(self, f):
        self.nVersion = int.from_bytes(f.read(4), "little", signed=True)
        self.nServices = int.from_bytes(f.read(8), "little")
        self.nTime = int.from_bytes(f.read(8), "little", signed=True)
        self.addrTo = CAddress()
        self.addrTo.deserialize(f, with_time=False)

        self.addrFrom = CAddress()
        self.addrFrom.deserialize(f, with_time=False)
        self.nNonce = int.from_bytes(f.read(8), "little")
        self.strSubVer = deser_string(f).decode('utf-8')

        self.nStartingHeight = int.from_bytes(f.read(4), "little", signed=True)

        # Relay field is optional for version 70001 onwards
        # But, unconditionally check it to match behaviour in freicoind
        self.relay = int.from_bytes(f.read(1), "little")  # f.read(1) may return an empty b''

    def serialize(self):
        r = b""
        r += self.nVersion.to_bytes(4, "little", signed=True)
        r += self.nServices.to_bytes(8, "little")
        r += self.nTime.to_bytes(8, "little", signed=True)
        r += self.addrTo.serialize(with_time=False)
        r += self.addrFrom.serialize(with_time=False)
        r += self.nNonce.to_bytes(8, "little")
        r += ser_string(self.strSubVer.encode('utf-8'))
        r += self.nStartingHeight.to_bytes(4, "little", signed=True)
        r += self.relay.to_bytes(1, "little")
        return r

    def __repr__(self):
        return 'msg_version(nVersion=%i nServices=%i nTime=%s addrTo=%s addrFrom=%s nNonce=0x%016X strSubVer=%s nStartingHeight=%i relay=%i)' \
            % (self.nVersion, self.nServices, time.ctime(self.nTime),
               repr(self.addrTo), repr(self.addrFrom), self.nNonce,
               self.strSubVer, self.nStartingHeight, self.relay)


class msg_verack:
    __slots__ = ()
    msgtype = b"verack"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_verack()"


class msg_addr:
    __slots__ = ("addrs",)
    msgtype = b"addr"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress)

    def serialize(self):
        return ser_vector(self.addrs)

    def __repr__(self):
        return "msg_addr(addrs=%s)" % (repr(self.addrs))


class msg_addrv2:
    __slots__ = ("addrs",)
    msgtype = b"addrv2"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress, "deserialize_v2")

    def serialize(self):
        return ser_vector(self.addrs, "serialize_v2")

    def __repr__(self):
        return "msg_addrv2(addrs=%s)" % (repr(self.addrs))


class msg_sendaddrv2:
    __slots__ = ()
    msgtype = b"sendaddrv2"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_sendaddrv2()"


class msg_inv:
    __slots__ = ("inv",)
    msgtype = b"inv"

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


class msg_getdata:
    __slots__ = ("inv",)
    msgtype = b"getdata"

    def __init__(self, inv=None):
        self.inv = inv if inv is not None else []

    def deserialize(self, f):
        self.inv = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.inv)

    def __repr__(self):
        return "msg_getdata(inv=%s)" % (repr(self.inv))


class msg_getblocks:
    __slots__ = ("locator", "hashstop")
    msgtype = b"getblocks"

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


class msg_tx:
    __slots__ = ("tx",)
    msgtype = b"tx"

    def __init__(self, tx=CTransaction()):
        self.tx = tx

    def deserialize(self, f):
        self.tx.deserialize(f)

    def serialize(self):
        return self.tx.serialize_with_witness()

    def __repr__(self):
        return "msg_tx(tx=%s)" % (repr(self.tx))

class msg_wtxidrelay:
    __slots__ = ()
    msgtype = b"wtxidrelay"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_wtxidrelay()"


class msg_no_witness_tx(msg_tx):
    __slots__ = ()

    def serialize(self):
        return self.tx.serialize_without_witness()


class msg_block:
    __slots__ = ("block",)
    msgtype = b"block"

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
# note that the user must supply the name of the msgtype, and the data
class msg_generic:
    __slots__ = ("data")

    def __init__(self, msgtype, data=None):
        self.msgtype = msgtype
        self.data = data

    def serialize(self):
        return self.data

    def __repr__(self):
        return "msg_generic()"


class msg_no_witness_block(msg_block):
    __slots__ = ()
    def serialize(self):
        return self.block.serialize(with_witness=False)


class msg_getaddr:
    __slots__ = ()
    msgtype = b"getaddr"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_getaddr()"


class msg_ping:
    __slots__ = ("nonce",)
    msgtype = b"ping"

    def __init__(self, nonce=0):
        self.nonce = nonce

    def deserialize(self, f):
        self.nonce = int.from_bytes(f.read(8), "little")

    def serialize(self):
        r = b""
        r += self.nonce.to_bytes(8, "little")
        return r

    def __repr__(self):
        return "msg_ping(nonce=%08x)" % self.nonce


class msg_pong:
    __slots__ = ("nonce",)
    msgtype = b"pong"

    def __init__(self, nonce=0):
        self.nonce = nonce

    def deserialize(self, f):
        self.nonce = int.from_bytes(f.read(8), "little")

    def serialize(self):
        r = b""
        r += self.nonce.to_bytes(8, "little")
        return r

    def __repr__(self):
        return "msg_pong(nonce=%08x)" % self.nonce


class msg_mempool:
    __slots__ = ()
    msgtype = b"mempool"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_mempool()"


class msg_notfound:
    __slots__ = ("vec", )
    msgtype = b"notfound"

    def __init__(self, vec=None):
        self.vec = vec or []

    def deserialize(self, f):
        self.vec = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.vec)

    def __repr__(self):
        return "msg_notfound(vec=%s)" % (repr(self.vec))


class msg_sendheaders:
    __slots__ = ()
    msgtype = b"sendheaders"

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
class msg_getheaders:
    __slots__ = ("hashstop", "locator",)
    msgtype = b"getheaders"

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
class msg_headers:
    __slots__ = ("headers",)
    msgtype = b"headers"

    def __init__(self, headers=None):
        self.headers = headers if headers is not None else []

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


class msg_merkleblock:
    __slots__ = ("merkleblock",)
    msgtype = b"merkleblock"

    def __init__(self, merkleblock=None):
        if merkleblock is None:
            self.merkleblock = CMerkleBlock()
        else:
            self.merkleblock = merkleblock

    def deserialize(self, f):
        self.merkleblock.deserialize(f)

    def serialize(self):
        return self.merkleblock.serialize()

    def __repr__(self):
        return "msg_merkleblock(merkleblock=%s)" % (repr(self.merkleblock))


class msg_filterload:
    __slots__ = ("data", "nHashFuncs", "nTweak", "nFlags")
    msgtype = b"filterload"

    def __init__(self, data=b'00', nHashFuncs=0, nTweak=0, nFlags=0):
        self.data = data
        self.nHashFuncs = nHashFuncs
        self.nTweak = nTweak
        self.nFlags = nFlags

    def deserialize(self, f):
        self.data = deser_string(f)
        self.nHashFuncs = int.from_bytes(f.read(4), "little")
        self.nTweak = int.from_bytes(f.read(4), "little")
        self.nFlags = int.from_bytes(f.read(1), "little")

    def serialize(self):
        r = b""
        r += ser_string(self.data)
        r += self.nHashFuncs.to_bytes(4, "little")
        r += self.nTweak.to_bytes(4, "little")
        r += self.nFlags.to_bytes(1, "little")
        return r

    def __repr__(self):
        return "msg_filterload(data={}, nHashFuncs={}, nTweak={}, nFlags={})".format(
            self.data, self.nHashFuncs, self.nTweak, self.nFlags)


class msg_filteradd:
    __slots__ = ("data")
    msgtype = b"filteradd"

    def __init__(self, data):
        self.data = data

    def deserialize(self, f):
        self.data = deser_string(f)

    def serialize(self):
        r = b""
        r += ser_string(self.data)
        return r

    def __repr__(self):
        return "msg_filteradd(data={})".format(self.data)


class msg_filterclear:
    __slots__ = ()
    msgtype = b"filterclear"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_filterclear()"


class msg_feefilter:
    __slots__ = ("feerate",)
    msgtype = b"feefilter"

    def __init__(self, feerate=0):
        self.feerate = feerate

    def deserialize(self, f):
        self.feerate = int.from_bytes(f.read(8), "little")

    def serialize(self):
        r = b""
        r += self.feerate.to_bytes(8, "little")
        return r

    def __repr__(self):
        return "msg_feefilter(feerate=%08x)" % self.feerate


class msg_sendcmpct:
    __slots__ = ("announce", "version")
    msgtype = b"sendcmpct"

    def __init__(self, announce=False, version=1):
        self.announce = announce
        self.version = version

    def deserialize(self, f):
        self.announce = bool(int.from_bytes(f.read(1), "little"))
        self.version = int.from_bytes(f.read(8), "little")

    def serialize(self):
        r = b""
        r += int(self.announce).to_bytes(1, "little")
        r += self.version.to_bytes(8, "little")
        return r

    def __repr__(self):
        return "msg_sendcmpct(announce=%s, version=%lu)" % (self.announce, self.version)


class msg_cmpctblock:
    __slots__ = ("header_and_shortids",)
    msgtype = b"cmpctblock"

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


class msg_getblocktxn:
    __slots__ = ("block_txn_request",)
    msgtype = b"getblocktxn"

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


class msg_blocktxn:
    __slots__ = ("block_transactions",)
    msgtype = b"blocktxn"

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


class msg_no_witness_blocktxn(msg_blocktxn):
    __slots__ = ()

    def serialize(self):
        return self.block_transactions.serialize(with_witness=False)


class msg_getcfilters:
    __slots__ = ("filter_type", "start_height", "stop_hash")
    msgtype =  b"getcfilters"

    def __init__(self, filter_type=None, start_height=None, stop_hash=None):
        self.filter_type = filter_type
        self.start_height = start_height
        self.stop_hash = stop_hash

    def deserialize(self, f):
        self.filter_type = int.from_bytes(f.read(1), "little")
        self.start_height = int.from_bytes(f.read(4), "little")
        self.stop_hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.filter_type.to_bytes(1, "little")
        r += self.start_height.to_bytes(4, "little")
        r += ser_uint256(self.stop_hash)
        return r

    def __repr__(self):
        return "msg_getcfilters(filter_type={:#x}, start_height={}, stop_hash={:x})".format(
            self.filter_type, self.start_height, self.stop_hash)

class msg_cfilter:
    __slots__ = ("filter_type", "block_hash", "filter_data")
    msgtype =  b"cfilter"

    def __init__(self, filter_type=None, block_hash=None, filter_data=None):
        self.filter_type = filter_type
        self.block_hash = block_hash
        self.filter_data = filter_data

    def deserialize(self, f):
        self.filter_type = int.from_bytes(f.read(1), "little")
        self.block_hash = deser_uint256(f)
        self.filter_data = deser_string(f)

    def serialize(self):
        r = b""
        r += self.filter_type.to_bytes(1, "little")
        r += ser_uint256(self.block_hash)
        r += ser_string(self.filter_data)
        return r

    def __repr__(self):
        return "msg_cfilter(filter_type={:#x}, block_hash={:x})".format(
            self.filter_type, self.block_hash)

class msg_getcfheaders:
    __slots__ = ("filter_type", "start_height", "stop_hash")
    msgtype =  b"getcfheaders"

    def __init__(self, filter_type=None, start_height=None, stop_hash=None):
        self.filter_type = filter_type
        self.start_height = start_height
        self.stop_hash = stop_hash

    def deserialize(self, f):
        self.filter_type = int.from_bytes(f.read(1), "little")
        self.start_height = int.from_bytes(f.read(4), "little")
        self.stop_hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.filter_type.to_bytes(1, "little")
        r += self.start_height.to_bytes(4, "little")
        r += ser_uint256(self.stop_hash)
        return r

    def __repr__(self):
        return "msg_getcfheaders(filter_type={:#x}, start_height={}, stop_hash={:x})".format(
            self.filter_type, self.start_height, self.stop_hash)

class msg_cfheaders:
    __slots__ = ("filter_type", "stop_hash", "prev_header", "hashes")
    msgtype =  b"cfheaders"

    def __init__(self, filter_type=None, stop_hash=None, prev_header=None, hashes=None):
        self.filter_type = filter_type
        self.stop_hash = stop_hash
        self.prev_header = prev_header
        self.hashes = hashes

    def deserialize(self, f):
        self.filter_type = int.from_bytes(f.read(1), "little")
        self.stop_hash = deser_uint256(f)
        self.prev_header = deser_uint256(f)
        self.hashes = deser_uint256_vector(f)

    def serialize(self):
        r = b""
        r += self.filter_type.to_bytes(1, "little")
        r += ser_uint256(self.stop_hash)
        r += ser_uint256(self.prev_header)
        r += ser_uint256_vector(self.hashes)
        return r

    def __repr__(self):
        return "msg_cfheaders(filter_type={:#x}, stop_hash={:x})".format(
            self.filter_type, self.stop_hash)

class msg_getcfcheckpt:
    __slots__ = ("filter_type", "stop_hash")
    msgtype =  b"getcfcheckpt"

    def __init__(self, filter_type=None, stop_hash=None):
        self.filter_type = filter_type
        self.stop_hash = stop_hash

    def deserialize(self, f):
        self.filter_type = int.from_bytes(f.read(1), "little")
        self.stop_hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.filter_type.to_bytes(1, "little")
        r += ser_uint256(self.stop_hash)
        return r

    def __repr__(self):
        return "msg_getcfcheckpt(filter_type={:#x}, stop_hash={:x})".format(
            self.filter_type, self.stop_hash)

class msg_cfcheckpt:
    __slots__ = ("filter_type", "stop_hash", "headers")
    msgtype =  b"cfcheckpt"

    def __init__(self, filter_type=None, stop_hash=None, headers=None):
        self.filter_type = filter_type
        self.stop_hash = stop_hash
        self.headers = headers

    def deserialize(self, f):
        self.filter_type = int.from_bytes(f.read(1), "little")
        self.stop_hash = deser_uint256(f)
        self.headers = deser_uint256_vector(f)

    def serialize(self):
        r = b""
        r += self.filter_type.to_bytes(1, "little")
        r += ser_uint256(self.stop_hash)
        r += ser_uint256_vector(self.headers)
        return r

    def __repr__(self):
        return "msg_cfcheckpt(filter_type={:#x}, stop_hash={:x})".format(
            self.filter_type, self.stop_hash)

class msg_sendtxrcncl:
    __slots__ = ("version", "salt")
    msgtype = b"sendtxrcncl"

    def __init__(self):
        self.version = 0
        self.salt = 0

    def deserialize(self, f):
        self.version = int.from_bytes(f.read(4), "little")
        self.salt = int.from_bytes(f.read(8), "little")

    def serialize(self):
        r = b""
        r += self.version.to_bytes(4, "little")
        r += self.salt.to_bytes(8, "little")
        return r

    def __repr__(self):
        return "msg_sendtxrcncl(version=%lu, salt=%lu)" %\
            (self.version, self.salt)

class TestFrameworkScript(unittest.TestCase):
    def test_addrv2_encode_decode(self):
        def check_addrv2(ip, net):
            addr = CAddress()
            addr.net, addr.ip = net, ip
            ser = addr.serialize_v2()
            actual = CAddress()
            actual.deserialize_v2(BytesIO(ser))
            self.assertEqual(actual, addr)

        check_addrv2("1.65.195.98", CAddress.NET_IPV4)
        check_addrv2("2001:41f0::62:6974:636f:696e", CAddress.NET_IPV6)
        check_addrv2("2bqghnldu6mcug4pikzprwhtjjnsyederctvci6klcwzepnjd46ikjyd.onion", CAddress.NET_TORV3)
        check_addrv2("255fhcp6ajvftnyo7bwz3an3t4a4brhopm3bamyh2iu5r3gnr2rq.b32.i2p", CAddress.NET_I2P)
        check_addrv2("fc32:17ea:e415:c3bf:9808:149d:b5a2:c9aa", CAddress.NET_CJDNS)
