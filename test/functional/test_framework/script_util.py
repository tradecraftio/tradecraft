#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
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
"""Useful Script constants and utils."""
from test_framework.script import CScript

# To prevent a "tx-size-small" policy rule error, a transaction has to have a
# non-witness size of at least 82 bytes (MIN_STANDARD_TX_NONWITNESS_SIZE in
# src/policy/policy.h). Considering a Tx with the smallest possible single
# input (blank, empty scriptSig), and with an output omitting the scriptPubKey,
# we get to a minimum size of 60 bytes:
#
# Tx Skeleton: 4 [Version] + 1 [InCount] + 1 [OutCount] + 4 [LockTime] = 10 bytes
# Blank Input: 32 [PrevTxHash] + 4 [Index] + 1 [scriptSigLen] + 4 [SeqNo] = 41 bytes
# Output:      8 [Amount] + 1 [scriptPubKeyLen] = 9 bytes
#
# Hence, the scriptPubKey of the single output has to have a size of at
# least 22 bytes, which corresponds to the size of a P2WPKH scriptPubKey.
# The following script constant consists of a single push of 21 bytes of 'a':
#   <PUSH_21> <21-bytes of 'a'>
# resulting in a 22-byte size. It should be used whenever (small) fake
# scriptPubKeys are needed, to guarantee that the minimum transaction size is
# met.
DUMMY_P2WPKH_SCRIPT = CScript([b'a' * 21])
