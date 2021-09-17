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
"""Test for the ZMQ RPC methods."""

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal


class RPCZMQTest(FreicoinTestFramework):

    address = "tcp://127.0.0.1:28332"

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_py3_zmq()
        self.skip_if_no_freicoind_zmq()

    def run_test(self):
        self._test_getzmqnotifications()

    def _test_getzmqnotifications(self):
        self.restart_node(0, extra_args=[])
        assert_equal(self.nodes[0].getzmqnotifications(), [])

        self.restart_node(0, extra_args=["-zmqpubhashtx=%s" % self.address])
        assert_equal(self.nodes[0].getzmqnotifications(), [
            {"type": "pubhashtx", "address": self.address},
        ])


if __name__ == '__main__':
    RPCZMQTest().main()
