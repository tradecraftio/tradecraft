#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
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
"""Test error messages for 'getaddressinfo' and 'validateaddress' RPC commands."""

from test_framework.test_framework import FreicoinTestFramework

from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

BECH32_VALID = 'fcrt1qtmp74ayg7p24uslctssvjm06q5phz4yr4g3036'
BECH32_VALID_CAPITALS = 'FCRT1QPLMTZKC2XHARPPZDLNPAQL78RSHJ68U3ZH5SU4'
BECH32_VALID_MULTISIG = 'fcrt1qdg3myrgvzw7ml9q0ejxhlkyxm7vl9r56yzkfgvzclrf4hkpx9yfqf50q49'

BECH32_INVALID_BECH32 = 'fcrt1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqxjh870'
BECH32_INVALID_BECH32M = 'fcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7khuzg9e'
BECH32_INVALID_VERSION = 'fcrt130xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vq06afwp'
BECH32_INVALID_SIZE = 'fcrt1s0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7v8n0nx0muaewav24umuen7l8wthtz45p3ftn58pvrs9xlumvkuu2xet8egzkcklqtes7d7vcfrh3dw'
BECH32_INVALID_V0_SIZE = 'fcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7kqqetwtn0'
BECH32_INVALID_PREFIX = 'fc1pw508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7kyf785h'
BECH32_TOO_LONG = 'fcrt1qppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp8ca2mx'
BECH32_ONE_ERROR = 'fcrt1qtmp74ayg7p24uslctssvjn06q5phz4yr4g3036'
BECH32_ONE_ERROR_CAPITALS = 'FCRT1QTMP74AYG7P24USLCTSSVJN06Q5PHZ4YR4G3036'
BECH32_TWO_ERRORS = 'fcrt1qax9suht3qv95sw33wavw8crpxduefdrslue3au' # should be fcrt1qax9suht3qv95sw33wavx8crpxduefdrslue3as
BECH32_NO_SEPARATOR = 'fcrtq049ldschfnwystcqnsvyfpj23mpsg3jc2efzy6'
BECH32_INVALID_CHAR = 'fcrt1q04oldschfnwystcqnsvyfpj23mpsg3jc2efzy6'
BECH32_MULTISIG_TWO_ERRORS = 'fcrt1qdg3myrgvzw7ml8q0ejxhlkyxn7vl9r56yzkfgvzclrf4hkpx9yfqf50q49'
BECH32_WRONG_VERSION = 'fcrt1ptmp74ayg7p24uslctssvjm06q5phz4yr4g3036'

BASE58_VALID = 'mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJRfn'
BASE58_INVALID_PREFIX = '17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem'
BASE58_INVALID_CHECKSUM = 'mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJJfn'
BASE58_INVALID_LENGTH = '2VKf7XKMrp4bVNVmuRbyCewkP8FhGLP2E54LHDPakr9Sq5mtU2'

INVALID_ADDRESS = 'asfah14i8fajz0123f'
INVALID_ADDRESS_2 = '1q049ldschfnwystcqnsvyfpj23mpsg3jcedq9xv'

class InvalidAddressErrorMessageTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def check_valid(self, addr):
        info = self.nodes[0].validateaddress(addr)
        assert info['isvalid']
        assert 'error' not in info
        assert 'error_locations' not in info

    def check_invalid(self, addr, error_str, error_locations=None):
        res = self.nodes[0].validateaddress(addr)
        assert not res['isvalid']
        assert_equal(res['error'], error_str)
        if error_locations:
            assert_equal(res['error_locations'], error_locations)
        else:
            assert_equal(res['error_locations'], [])

    def test_validateaddress(self):
        # Invalid Bech32
        self.check_invalid(BECH32_INVALID_SIZE, 'Bech32 string too long', error_locations=list(range(90, 134)))
        self.check_invalid(BECH32_INVALID_PREFIX, 'Invalid or unsupported Segwit (Bech32) or Base58 encoding.')
        self.check_invalid(BECH32_INVALID_BECH32, 'All address types must use Bech32m checksum')
        self.check_valid(BECH32_INVALID_BECH32M) # Switch to bech32m for all segwit addresses, and change prefix.
        self.check_valid(BECH32_INVALID_VERSION) # Expand allowed witness versions to be any of the 31 1-byte opcodes which can appear at the beginning of a legacy, pre-cleanup scriptPubKey.
        self.check_valid(BECH32_INVALID_V0_SIZE) # Unrecognized witness lengths are treated as unknown segwit versions and return true
        self.check_invalid(BECH32_TOO_LONG, 'Bech32 string too long', [90])
        self.check_invalid(BECH32_ONE_ERROR, 'Invalid Bech32m checksum', [27])
        self.check_invalid(BECH32_TWO_ERRORS, 'Invalid Bech32m checksum', [25, 43])
        self.check_invalid(BECH32_ONE_ERROR_CAPITALS, 'Invalid Bech32m checksum', [27])
        self.check_invalid(BECH32_NO_SEPARATOR, 'Missing separator')
        self.check_invalid(BECH32_INVALID_CHAR, 'Invalid Base 32 character', [8])
        self.check_invalid(BECH32_MULTISIG_TWO_ERRORS, 'Invalid Bech32m checksum', [19, 30])
        self.check_invalid(BECH32_WRONG_VERSION, 'Invalid Bech32m checksum', [5])

        # Valid Bech32
        self.check_valid(BECH32_VALID)
        self.check_valid(BECH32_VALID_CAPITALS)
        self.check_valid(BECH32_VALID_MULTISIG)

        # Invalid Base58
        self.check_invalid(BASE58_INVALID_PREFIX, 'Invalid or unsupported Base58-encoded address.')
        self.check_invalid(BASE58_INVALID_CHECKSUM, 'Invalid checksum or length of Base58 address')
        self.check_invalid(BASE58_INVALID_LENGTH, 'Invalid checksum or length of Base58 address')

        # Valid Base58
        self.check_valid(BASE58_VALID)

        # Invalid address format
        self.check_invalid(INVALID_ADDRESS, 'Invalid or unsupported Segwit (Bech32) or Base58 encoding.')
        self.check_invalid(INVALID_ADDRESS_2, 'Invalid or unsupported Segwit (Bech32) or Base58 encoding.')

    def test_getaddressinfo(self):
        node = self.nodes[0]

        assert_raises_rpc_error(-5, "Bech32 string too long", node.getaddressinfo, BECH32_INVALID_SIZE)

        assert_raises_rpc_error(-5, "Invalid or unsupported Segwit (Bech32) or Base58 encoding.", node.getaddressinfo, BECH32_INVALID_PREFIX)

        assert_raises_rpc_error(-5, "Invalid or unsupported Base58-encoded address.", node.getaddressinfo, BASE58_INVALID_PREFIX)

        assert_raises_rpc_error(-5, "Invalid or unsupported Segwit (Bech32) or Base58 encoding.", node.getaddressinfo, INVALID_ADDRESS)

    def run_test(self):
        self.test_validateaddress()

        if self.is_wallet_compiled():
            self.init_wallet(node=0)
            self.test_getaddressinfo()


if __name__ == '__main__':
    InvalidAddressErrorMessageTest().main()
