#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Copyright (c) 2010-2023 The Freicoin Developers
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

import base64

from .messages import (
    CTransaction,
    deser_string,
    from_binary,
    ser_compact_size,
)


# global types
PST_GLOBAL_UNSIGNED_TX = 0x00
PST_GLOBAL_XPUB = 0x01
PST_GLOBAL_TX_VERSION = 0x02
PST_GLOBAL_FALLBACK_LOCKTIME = 0x03
PST_GLOBAL_INPUT_COUNT = 0x04
PST_GLOBAL_OUTPUT_COUNT = 0x05
PST_GLOBAL_TX_MODIFIABLE = 0x06
PST_GLOBAL_VERSION = 0xfb
PST_GLOBAL_PROPRIETARY = 0xfc

# per-input types
PST_IN_NON_WITNESS_UTXO = 0x00
PST_IN_WITNESS_UTXO = 0x01
PST_IN_PARTIAL_SIG = 0x02
PST_IN_SIGHASH_TYPE = 0x03
PST_IN_REDEEM_SCRIPT = 0x04
PST_IN_WITNESS_SCRIPT = 0x05
PST_IN_BIP32_DERIVATION = 0x06
PST_IN_FINAL_SCRIPTSIG = 0x07
PST_IN_FINAL_SCRIPTWITNESS = 0x08
PST_IN_POR_COMMITMENT = 0x09
PST_IN_RIPEMD160 = 0x0a
PST_IN_SHA256 = 0x0b
PST_IN_HASH160 = 0x0c
PST_IN_HASH256 = 0x0d
PST_IN_PREVIOUS_TXID = 0x0e
PST_IN_OUTPUT_INDEX = 0x0f
PST_IN_SEQUENCE = 0x10
PST_IN_REQUIRED_TIME_LOCKTIME = 0x11
PST_IN_REQUIRED_HEIGHT_LOCKTIME = 0x12
PST_IN_TAP_KEY_SIG = 0x13
PST_IN_TAP_SCRIPT_SIG = 0x14
PST_IN_TAP_LEAF_SCRIPT = 0x15
PST_IN_TAP_BIP32_DERIVATION = 0x16
PST_IN_TAP_INTERNAL_KEY = 0x17
PST_IN_TAP_MERKLE_ROOT = 0x18
PST_IN_PROPRIETARY = 0xfc

# per-output types
PST_OUT_REDEEM_SCRIPT = 0x00
PST_OUT_WITNESS_SCRIPT = 0x01
PST_OUT_BIP32_DERIVATION = 0x02
PST_OUT_AMOUNT = 0x03
PST_OUT_SCRIPT = 0x04
PST_OUT_TAP_INTERNAL_KEY = 0x05
PST_OUT_TAP_TREE = 0x06
PST_OUT_TAP_BIP32_DERIVATION = 0x07
PST_OUT_PROPRIETARY = 0xfc


class PSTMap:
    """Class for serializing and deserializing PST maps"""

    def __init__(self, map=None):
        self.map = map if map is not None else {}

    def deserialize(self, f):
        m = {}
        while True:
            k = deser_string(f)
            if len(k) == 0:
                break
            v = deser_string(f)
            if len(k) == 1:
                k = k[0]
            assert k not in m
            m[k] = v
        self.map = m

    def serialize(self):
        m = b""
        for k,v in self.map.items():
            if isinstance(k, int) and 0 <= k and k <= 255:
                k = bytes([k])
            m += ser_compact_size(len(k)) + k
            m += ser_compact_size(len(v)) + v
        m += b"\x00"
        return m

class PST:
    """Class for serializing and deserializing PSTs"""

    def __init__(self, *, g=None, i=None, o=None):
        self.g = g if g is not None else PSTMap()
        self.i = i if i is not None else []
        self.o = o if o is not None else []
        self.tx = None

    def deserialize(self, f):
        assert f.read(5) == b"psbt\xff"
        self.g = from_binary(PSTMap, f)
        assert 0 in self.g.map
        self.tx = from_binary(CTransaction, self.g.map[0])
        self.i = [from_binary(PSTMap, f) for _ in self.tx.vin]
        self.o = [from_binary(PSTMap, f) for _ in self.tx.vout]
        return self

    def serialize(self):
        assert isinstance(self.g, PSTMap)
        assert isinstance(self.i, list) and all(isinstance(x, PSTMap) for x in self.i)
        assert isinstance(self.o, list) and all(isinstance(x, PSTMap) for x in self.o)
        assert 0 in self.g.map
        tx = from_binary(CTransaction, self.g.map[0])
        assert len(tx.vin) == len(self.i)
        assert len(tx.vout) == len(self.o)

        pst = [x.serialize() for x in [self.g] + self.i + self.o]
        return b"psbt\xff" + b"".join(pst)

    def make_blank(self):
        """
        Remove all fields except for PST_GLOBAL_UNSIGNED_TX
        """
        for m in self.i + self.o:
            m.map.clear()

        self.g = PSTMap(map={0: self.g.map[0]})

    def to_base64(self):
        return base64.b64encode(self.serialize()).decode("utf8")

    @classmethod
    def from_base64(cls, b64pst):
        return from_binary(cls, base64.b64decode(b64pst))
