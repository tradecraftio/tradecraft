#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
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

#
# Test -alertnotify 
#

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import *

class ForkNotifyTest(FreicoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = False

    alert_filename = None  # Set by setup_network

    def setup_network(self):
        self.nodes = []
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        with open(self.alert_filename, 'w', encoding='utf8'):
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
        self.nodes[1].generate(51)
        self.sync_all()
        # -alertnotify should trigger on the 51'st,
        # but mine and sync another to give
        # -alertnotify time to write
        self.nodes[1].generate(1)
        self.sync_all()

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text = f.read()

        if len(alert_text) == 0:
            raise AssertionError("-alertnotify did not warn of up-version blocks")

        # Mine more up-version blocks, should not get more alerts:
        self.nodes[1].generate(1)
        self.sync_all()
        self.nodes[1].generate(1)
        self.sync_all()

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text2 = f.read()

        if alert_text != alert_text2:
            raise AssertionError("-alertnotify excessive warning of up-version blocks")

if __name__ == '__main__':
    ForkNotifyTest().main()
