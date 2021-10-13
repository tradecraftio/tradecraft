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
"""Test RPC misc output."""
import xml.etree.ElementTree as ET

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)

from test_framework.authproxy import JSONRPCException


class RpcMiscTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]

        self.log.info("test getmemoryinfo")
        memory = node.getmemoryinfo()['locked']
        assert_greater_than(memory['used'], 0)
        assert_greater_than(memory['free'], 0)
        assert_greater_than(memory['total'], 0)
        # assert_greater_than_or_equal() for locked in case locking pages failed at some point
        assert_greater_than_or_equal(memory['locked'], 0)
        assert_greater_than(memory['chunks_used'], 0)
        assert_greater_than(memory['chunks_free'], 0)
        assert_equal(memory['used'] + memory['free'], memory['total'])

        self.log.info("test mallocinfo")
        try:
            mallocinfo = node.getmemoryinfo(mode="mallocinfo")
            self.log.info('getmemoryinfo(mode="mallocinfo") call succeeded')
            tree = ET.fromstring(mallocinfo)
            assert_equal(tree.tag, 'malloc')
        except JSONRPCException:
            self.log.info('getmemoryinfo(mode="mallocinfo") not available')
            assert_raises_rpc_error(-8, 'mallocinfo is only available when compiled with glibc 2.10+', node.getmemoryinfo, mode="mallocinfo")

        assert_raises_rpc_error(-8, "unknown mode foobar", node.getmemoryinfo, mode="foobar")

if __name__ == '__main__':
    RpcMiscTest().main()
