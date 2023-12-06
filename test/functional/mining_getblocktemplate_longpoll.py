#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
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
"""Test longpolling with getblocktemplate."""

import random
import threading

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import get_rpc_proxy
from test_framework.wallet import MiniWallet


class LongpollThread(threading.Thread):
    def __init__(self, node):
        threading.Thread.__init__(self)
        # query current longpollid
        template = node.getblocktemplate({'rules': ['segwit','finaltx']})
        self.longpollid = template['longpollid']
        # create a new connection to the node, we can't use the same
        # connection from two threads
        self.node = get_rpc_proxy(node.url, 1, timeout=600, coveragedir=node.coverage_dir)

    def run(self):
        self.node.getblocktemplate({'longpollid': self.longpollid, 'rules': ['segwit','finaltx']})

class GetBlockTemplateLPTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def run_test(self):
        self.log.info("Warning: this test will take about 70 seconds in the best case. Be patient.")
        self.log.info("Test that longpollid doesn't change between successive getblocktemplate() invocations if nothing else happens")
        self.generate(self.nodes[0], 10)
        template = self.nodes[0].getblocktemplate({'rules': ['segwit','finaltx']})
        longpollid = template['longpollid']
        template2 = self.nodes[0].getblocktemplate({'rules': ['segwit','finaltx']})
        assert template2['longpollid'] == longpollid

        self.log.info("Test that longpoll waits if we do nothing")
        thr = LongpollThread(self.nodes[0])
        with self.nodes[0].assert_debug_log(["ThreadRPCServer method=getblocktemplate"], timeout=3):
            thr.start()
        # check that thread still lives
        thr.join(5)  # wait 5 seconds or until thread exits
        assert thr.is_alive()

        self.miniwallet = MiniWallet(self.nodes[0])
        self.log.info("Test that longpoll will terminate if another node generates a block")
        self.generate(self.nodes[1], 1)  # generate a block on another node
        # check that thread will exit now that new transaction entered mempool
        thr.join(5)  # wait 5 seconds or until thread exits
        assert not thr.is_alive()

        self.log.info("Test that longpoll will terminate if we generate a block ourselves")
        thr = LongpollThread(self.nodes[0])
        with self.nodes[0].assert_debug_log(["ThreadRPCServer method=getblocktemplate"], timeout=3):
            thr.start()
        self.generate(self.nodes[0], 1)  # generate a block on own node
        thr.join(5)  # wait 5 seconds or until thread exits
        assert not thr.is_alive()

        self.log.info("Test that introducing a new transaction into the mempool will terminate the longpoll")
        thr = LongpollThread(self.nodes[0])
        with self.nodes[0].assert_debug_log(["ThreadRPCServer method=getblocktemplate"], timeout=3):
            thr.start()
        # generate a transaction and submit it
        self.miniwallet.send_self_transfer(from_node=random.choice(self.nodes))
        # after one minute, every 10 seconds the mempool is probed, so in 80 seconds it should have returned
        thr.join(60 + 20)
        assert not thr.is_alive()

if __name__ == '__main__':
    GetBlockTemplateLPTest().main()
