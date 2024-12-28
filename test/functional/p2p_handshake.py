#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
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
"""
Test P2P behaviour during the handshake phase.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import p2p_port


class P2PHandshakeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Check that connecting to ourself leads to immediate disconnect")
        with node.assert_debug_log(["connected to self", "disconnecting"]):
            node_listen_addr = f"127.0.0.1:{p2p_port(0)}"
            node.addconnection(node_listen_addr, "outbound-full-relay", self.options.v2transport)
            self.wait_until(lambda: len(node.getpeerinfo()) == 0)


if __name__ == '__main__':
    P2PHandshakeTest().main()
