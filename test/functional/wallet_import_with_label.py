#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
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
"""Test the behavior of RPC importprivkey on set and unset labels of
addresses.

It tests different cases in which an address is imported with importaddress
with or without a label and then its private key is imported with importprivkey
with and without a label.
"""

from test_framework.address import key_to_p2pkh
from test_framework.test_framework import FreicoinTestFramework
from test_framework.wallet_util import test_address


class ImportWithLabel(FreicoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=False)

    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        """Main test logic"""

        self.log.info(
            "Test importaddress with label and importprivkey without label."
        )
        self.log.info("Import a watch-only address with a label.")
        address = self.nodes[0].getnewaddress()
        label = "Test Label"
        self.nodes[1].importaddress(address, label)
        test_address(self.nodes[1],
                     address,
                     iswatchonly=True,
                     ismine=False,
                     labels=[label])

        self.log.info(
            "Import the watch-only address's private key without a "
            "label and the address should keep its label."
        )
        priv_key = self.nodes[0].dumpprivkey(address)
        self.nodes[1].importprivkey(priv_key)
        test_address(self.nodes[1], address, labels=[label])

        self.log.info(
            "Test importaddress without label and importprivkey with label."
        )
        self.log.info("Import a watch-only address without a label.")
        address2 = self.nodes[0].getnewaddress()
        self.nodes[1].importaddress(address2)
        test_address(self.nodes[1],
                     address2,
                     iswatchonly=True,
                     ismine=False,
                     labels=[""])

        self.log.info(
            "Import the watch-only address's private key with a "
            "label and the address should have its label updated."
        )
        priv_key2 = self.nodes[0].dumpprivkey(address2)
        label2 = "Test Label 2"
        self.nodes[1].importprivkey(priv_key2, label2)

        test_address(self.nodes[1], address2, labels=[label2])

        self.log.info("Test importaddress with label and importprivkey with label.")
        self.log.info("Import a watch-only address with a label.")
        address3 = self.nodes[0].getnewaddress()
        label3_addr = "Test Label 3 for importaddress"
        self.nodes[1].importaddress(address3, label3_addr)
        test_address(self.nodes[1],
                     address3,
                     iswatchonly=True,
                     ismine=False,
                     labels=[label3_addr])

        self.log.info(
            "Import the watch-only address's private key with a "
            "label and the address should have its label updated."
        )
        priv_key3 = self.nodes[0].dumpprivkey(address3)
        label3_priv = "Test Label 3 for importprivkey"
        self.nodes[1].importprivkey(priv_key3, label3_priv)

        test_address(self.nodes[1], address3, labels=[label3_priv])

        self.log.info(
            "Test importprivkey won't label new dests with the same "
            "label as others labeled dests for the same key."
        )
        self.log.info("Import a watch-only bech32 address with a label.")
        address4 = self.nodes[0].getnewaddress("", "bech32")
        label4_addr = "Test Label 4 for importaddress"
        self.nodes[1].importaddress(address4, label4_addr)
        test_address(self.nodes[1],
                     address4,
                     iswatchonly=True,
                     ismine=False,
                     labels=[label4_addr],
                     embedded=None)

        self.log.info(
            "Import the watch-only address's private key without a "
            "label and new destinations for the key should have an "
            "empty label while the 'old' destination should keep "
            "its label."
        )
        priv_key4 = self.nodes[0].dumpprivkey(address4)
        self.nodes[1].importprivkey(priv_key4)
        embedded_addr = key_to_p2pkh(self.nodes[1].getaddressinfo(address4)['embedded']['pubkey'])

        test_address(self.nodes[1], embedded_addr, labels=[""])

        test_address(self.nodes[1], address4, labels=[label4_addr])

        self.stop_nodes()


if __name__ == "__main__":
    ImportWithLabel().main()
