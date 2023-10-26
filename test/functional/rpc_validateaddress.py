#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
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
"""Test validateaddress for main chain"""

from test_framework.test_framework import FreicoinTestFramework

from test_framework.util import assert_equal

INVALID_DATA = [
    # BIP 173
    (
        "tf1qw508d6qejxtdg4y5r3zarvary0c5xw7kz03qzv",
        "Invalid or unsupported Segwit (Bech32) or Base58 encoding.",  # Invalid hrp
        [],
    ),
    (
        "fc1qw508d6qejxtdg4y5r3zarvary0c5xw7krlt8wl",
        "Invalid Bech32m checksum",
        [41]),
    (
        "FC13W508D6QEJXTDG4Y5R3ZARVARY0C5XW7KF3A5FT",
        "All address types must use Bech32m checksum",
        [],
    ),
    (
        "fc1rw5v5eqgn",
        "All address types must use Bech32m checksum",  # Invalid program length
        [],
    ),
    (
        "fc10w508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7kw5srplwz",
        "All address types must use Bech32m checksum",  # Invalid program length
        [],
    ),
    (
        "tf1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3qtmut5m",
        "Invalid or unsupported Segwit (Bech32) or Base58 encoding.",  # tf1, Mixed case
        [],
    ),
    (
        "FC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KKRMTt5",
        "Invalid character or mixed case",  # fc1, Mixed case, not in BIP 173 test vectors
        [40],
    ),
    (
        "fc1zw508d6qejxtdg4y5r3zarvaryvqtlcha7",
        "Invalid padding in Bech32 data section",  # Wrong padding
        [],
    ),
    (
        "tf1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3pr3cjvt",
        "Invalid or unsupported Segwit (Bech32) or Base58 encoding.",  # tf1, Non-zero padding in 8-to-5 conversion
        [],
    ),
    (
        "fc1ggn6yx",
        "Empty Bech32 data section",
        [],
    ),
    # BIP 350
    (
        "tc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vq5zuyut",
        "Invalid or unsupported Segwit (Bech32) or Base58 encoding.",  # Invalid human-readable part
        [],
    ),
    (
        "fc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqnre6ah",
        "All address types must use Bech32m checksum",  # Invalid checksum (Bech32 instead of Bech32m)
        [],
    ),
    (
        "tf1z0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqv5gppv",
        "Invalid or unsupported Segwit (Bech32) or Base58 encoding.",  # tf1, Invalid checksum (Bech32 instead of Bech32m)
        [],
    ),
    (
        "FC1S0XLXVLHEMJA6C4DQV22UAPCTQUPFHLXM9H8Z3K2E72Q4K9HCZ7VQSUNA49",
        "All address types must use Bech32m checksum",  # Invalid checksum (Bech32 instead of Bech32m)
        [],
    ),
    (
        "fc1qw508d6qejxtdg4y5r3zarvary0c5xw7kkrmtt5",
        "All address types must use Bech32m checksum",  # Invalid checksum (Bech32 instead of Bech32m)
        [],
    ),
    (
        "tf1q0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqmzptje",
        "Invalid or unsupported Segwit (Bech32) or Base58 encoding.",  # tf1, Invalid checksum (Bech32 instead of Bech32m)
        [],
    ),
    (
        "fc1p38j9r5y49hruaue7wxjce0updqjuyyx0kh56v8s25huc6995vvpql3jow4",
        "Invalid Base 32 character",  # Invalid character in checksum
        [59],
    ),
    (
        "fc1pw5avmtkg",
        "Invalid Bech32 address program size (1 byte)",
        [],
    ),
    (
        "tf1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vq34pZ29",
        "Invalid or unsupported Segwit (Bech32) or Base58 encoding.",  # tf1, Mixed case
        [],
    ),
    (
        "fc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7v07q98dg97",
        "Invalid padding in Bech32 data section",  # zero padding of more than 4 bits
        [],
    ),
    (
        "tf1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vpvr4hhh",
        "Invalid or unsupported Segwit (Bech32) or Base58 encoding.",  # tf1, Non-zero padding in 8-to-5 conversion
        [],
    ),
]
VALID_DATA = [
    (
        "FC130XLXVLHEMJA6C4DQV22UAPCTQUPFHLXM9H8Z3K2E72Q4K9HCZ7VQ6TN5DE",
        "602079be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798",
    ),
    (
        "fc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7v8n0nx0muaewav25z7mkkp",
        "4f2979be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f8179879be667ef9dcbbac55",
    ),
    (
        "FC1QR508D6QEJXTDG4Y5R3ZARVARYVSPEL2K",
        "00101d1e76e8199196d454941c45d1b3a323",
    ),
    # BIP 350
    (
        "FC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KRLT8WK",
        "0014751e76e8199196d454941c45d1b3a323f1433bd6",
    ),
    # (
    #   "tf1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q78v83e",
    #   "00201863143c14c5166804bd19203356da136c985678cd4d27a1b8c6329604903262",
    # ),
    (
        "fc1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3qfdynrf",
        "00201863143c14c5166804bd19203356da136c985678cd4d27a1b8c6329604903262",
    ),
    (
        "fc1pw508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7kyf785h",
        "4f28751e76e8199196d454941c45d1b3a323f1433bd6751e76e8199196d454941c45d1b3a323f1433bd6",
    ),
    (
        "FC1SW50QE2JK0N",
        "5f02751e",
    ),
    (
        "fc1zw508d6qejxtdg4y5r3zarvaryvau8qj9",
        "5110751e76e8199196d454941c45d1b3a323",
    ),
    # (
    #   "tf1qqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesj3yfsr",
    #   "0020000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
    # ),
    (
        "fc1qqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvses9mvazn",
        "0020000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
    ),
    # (
    #   "tf1pqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesd65vda",
    #   "4f20000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
    # ),
    (
        "fc1pqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvses6sucld",
        "4f20000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
    ),
    (
        "fc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqxlfkc4",
        "4f2079be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798",
    ),
]


class ValidateAddressMainTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.chain = ""  # main
        self.num_nodes = 1
        self.extra_args = [["-prune=899"]] * self.num_nodes

    def check_valid(self, addr, spk):
        info = self.nodes[0].validateaddress(addr)
        assert_equal(info["isvalid"], True)
        assert_equal(info["scriptPubKey"], spk)
        assert "error" not in info
        assert "error_locations" not in info

    def check_invalid(self, addr, error_str, error_locations):
        res = self.nodes[0].validateaddress(addr)
        assert_equal(res["isvalid"], False)
        assert_equal(res["error"], error_str)
        assert_equal(res["error_locations"], error_locations)

    def test_validateaddress(self):
        for (addr, error, locs) in INVALID_DATA:
            self.check_invalid(addr, error, locs)
        for (addr, spk) in VALID_DATA:
            self.check_valid(addr, spk)

    def run_test(self):
        self.test_validateaddress()


if __name__ == "__main__":
    ValidateAddressMainTest().main()
