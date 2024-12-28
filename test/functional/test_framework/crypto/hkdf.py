#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
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

"""Test-only HKDF-SHA256 implementation

It is designed for ease of understanding, not performance.

WARNING: This code is slow and trivially vulnerable to side channel attacks. Do not use for
anything but tests.
"""

import hashlib
import hmac


def hmac_sha256(key, data):
    """Compute HMAC-SHA256 from specified byte arrays key and data."""
    return hmac.new(key, data, hashlib.sha256).digest()


def hkdf_sha256(length, ikm, salt, info):
    """Derive a key using HKDF-SHA256."""
    if len(salt) == 0:
        salt = bytes([0] * 32)
    prk = hmac_sha256(salt, ikm)
    t = b""
    okm = b""
    for i in range((length + 32 - 1) // 32):
        t = hmac_sha256(prk, t + info + bytes([i + 1]))
        okm += t
    return okm[:length]
