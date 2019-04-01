#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Freicoin developers
# Copyright (c) 2010-2019 The Freicoin Developers
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
# Test RPC calls related to blockchain state. Tests correspond to code in
# rpcblockchain.cpp.
#

from decimal import Decimal

from test_framework.test_framework import FreicoinTestFramework
from test_framework.authproxy import JSONRPCException
from test_framework.util import (
    initialize_chain,
    assert_equal,
    assert_raises,
    assert_is_hex_string,
    assert_is_hash_string,
    start_nodes,
    connect_nodes_bi,
)


class BlockchainTest(FreicoinTestFramework):
    """
    Test blockchain-related RPC calls:

        - gettxoutsetinfo
        - verifychain

    """

    def setup_chain(self):
        print("Initializing test directory " + self.options.tmpdir)
        initialize_chain(self.options.tmpdir, bitcoinmode=self.options.bitcoinmode)

    def setup_network(self, split=False):
        self.nodes = start_nodes(2, self.options.tmpdir)
        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        self._test_gettxoutsetinfo()
        self._test_getblockheader()
        self.nodes[0].verifychain(4, 0)

    def _test_gettxoutsetinfo(self):
        node = self.nodes[0]
        res = node.gettxoutsetinfo()

        assert_equal(res['total_value'], Decimal('10000.00000000'))
        assert_equal(res['total_amount'], Decimal('9999.04161694'))
        assert_equal(res['transactions'], 200)
        assert_equal(res['height'], 200)
        assert_equal(res['txouts'], 200)
        assert_equal(res['bytes_serialized'], 14146),
        assert_equal(len(res['bestblock']), 64)
        assert_equal(len(res['hash_serialized']), 64)

    def _test_getblockheader(self):
        node = self.nodes[0]

        assert_raises(
            JSONRPCException, lambda: node.getblockheader('nonsense'))

        besthash = node.getbestblockhash()
        secondbesthash = node.getblockhash(199)
        header = node.getblockheader(besthash)

        assert_equal(header['hash'], besthash)
        assert_equal(header['height'], 200)
        assert_equal(header['confirmations'], 1)
        assert_equal(header['previousblockhash'], secondbesthash)
        assert_is_hex_string(header['chainwork'])
        assert_is_hash_string(header['hash'])
        assert_is_hash_string(header['previousblockhash'])
        assert_is_hash_string(header['merkleroot'])
        assert_is_hash_string(header['bits'], length=None)
        assert isinstance(header['time'], int)
        assert isinstance(header['mediantime'], int)
        assert isinstance(header['nonce'], int)
        assert isinstance(header['version'], int)
        assert isinstance(header['difficulty'], Decimal)

if __name__ == '__main__':
    BlockchainTest().main()
