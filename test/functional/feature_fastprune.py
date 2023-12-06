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
"""Test fastprune mode."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal
)
from test_framework.wallet import MiniWallet


class FeatureFastpruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-fastprune"]]

    def run_test(self):
        self.log.info("ensure that large blocks don't crash or freeze in -fastprune")
        wallet = MiniWallet(self.nodes[0])
        tx = wallet.create_self_transfer()['tx']
        annex = b"\x50" + b"\xff" * 0x10000
        tx.wit.vtxinwit[0].scriptWitness.stack.append(annex)
        self.generateblock(self.nodes[0], output="raw(55)", transactions=[tx.serialize().hex()])
        assert_equal(self.nodes[0].getblockcount(), 201)


if __name__ == '__main__':
    FeatureFastpruneTest().main()
