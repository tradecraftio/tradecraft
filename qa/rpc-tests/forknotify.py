#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Copyright (c) 2010-2019 The Freicoin developers
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
# Test -alertnotify 
#

from test_framework import FreicoinTestFramework
from freicoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *
import os
import shutil

class ForkNotifyTest(FreicoinTestFramework):

    alert_filename = None  # Set by setup_network

    def setup_network(self):
        self.nodes = []
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        with open(self.alert_filename, 'w') as f:
            pass  # Just open then close to create zero-length file
        self.nodes.append(start_node(0, self.options.tmpdir,
                            ["-blockversion=2", "-alertnotify=echo %s >> \"" + self.alert_filename + "\""]))
        # Node1 mines block.version=211 blocks
        self.nodes.append(start_node(1, self.options.tmpdir,
                                ["-blockversion=211"]))
        connect_nodes(self.nodes[1], 0)

        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        # Mine 51 up-version blocks
        self.nodes[1].setgenerate(True, 51)
        self.sync_all()
        # -alertnotify should trigger on the 51'st,
        # but mine and sync another to give
        # -alertnotify time to write
        self.nodes[1].setgenerate(True, 1)
        self.sync_all()

        with open(self.alert_filename, 'r') as f:
            alert_text = f.read()

        if len(alert_text) == 0:
            raise AssertionError("-alertnotify did not warn of up-version blocks")

        # Mine more up-version blocks, should not get more alerts:
        self.nodes[1].setgenerate(True, 1)
        self.sync_all()
        self.nodes[1].setgenerate(True, 1)
        self.sync_all()

        with open(self.alert_filename, 'r') as f:
            alert_text2 = f.read()

        if alert_text != alert_text2:
            raise AssertionError("-alertnotify excessive warning of up-version blocks")

if __name__ == '__main__':
    ForkNotifyTest().main()
