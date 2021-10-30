#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
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
"""Test RPC help output."""

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

import os


class HelpRpcTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.test_categories()
        self.dump_help()

    def test_categories(self):
        node = self.nodes[0]

        # wrong argument count
        assert_raises_rpc_error(-1, 'help', node.help, 'foo', 'bar')

        # invalid argument
        assert_raises_rpc_error(-1, 'JSON value is not a string as expected', node.help, 0)

        # help of unknown command
        assert_equal(node.help('foo'), 'help: unknown command: foo')

        # command titles
        titles = [line[3:-3] for line in node.help().splitlines() if line.startswith('==')]

        components = ['Blockchain', 'Control', 'Generating', 'Mining', 'Network', 'Rawtransactions', 'Util']

        if self.is_wallet_compiled():
            components.append('Wallet')

        if self.is_zmq_compiled():
            components.append('Zmq')

        assert_equal(titles, components)

    def dump_help(self):
        dump_dir = os.path.join(self.options.tmpdir, 'rpc_help_dump')
        os.mkdir(dump_dir)
        calls = [line.split(' ', 1)[0] for line in self.nodes[0].help().splitlines() if line and not line.startswith('==')]
        for call in calls:
            with open(os.path.join(dump_dir, call), 'w', encoding='utf-8') as f:
                # Make sure the node can generate the help at runtime without crashing
                f.write(self.nodes[0].help(call))


if __name__ == '__main__':
    HelpRpcTest().main()
