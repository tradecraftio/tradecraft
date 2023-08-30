#!/usr/bin/env python3
# Copyright (c) 2016-2020 The Bitcoin Core developers
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
"""Test using named arguments for RPCs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class NamedArgumentTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.supports_cli = False

    def run_test(self):
        node = self.nodes[0]
        h = node.help(command='getblockchaininfo')
        assert h.startswith('getblockchaininfo\n')

        assert_raises_rpc_error(-8, 'Unknown named parameter', node.help, random='getblockchaininfo')

        h = node.getblockhash(height=0)
        node.getblock(blockhash=h)

        assert_equal(node.echo(), [])
        assert_equal(node.echo(arg0=0,arg9=9), [0] + [None]*8 + [9])
        assert_equal(node.echo(arg1=1), [None, 1])
        assert_equal(node.echo(arg9=None), [None]*10)
        assert_equal(node.echo(arg0=0,arg3=3,arg9=9), [0] + [None]*2 + [3] + [None]*5 + [9])
        assert_equal(node.echo(0, 1, arg3=3, arg5=5), [0, 1, None, 3, None, 5])
        assert_raises_rpc_error(-8, "Parameter arg1 specified twice both as positional and named argument", node.echo, 0, 1, arg1=1)
        assert_raises_rpc_error(-8, "Parameter arg1 specified twice both as positional and named argument", node.echo, 0, None, 2, arg1=1)

if __name__ == '__main__':
    NamedArgumentTest().main()
