#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Core developers
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
"""Test p2p mempool message.

Test that nodes are disconnected if they send mempool messages when bloom
filters are not enabled.
"""

from test_framework.mininode import *
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import *

class P2PMempoolTests(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-peerbloomfilters=0"]]

    def run_test(self):
        # Add a p2p connection
        self.nodes[0].add_p2p_connection(P2PInterface())
        network_thread_start()
        self.nodes[0].p2p.wait_for_verack()

        #request mempool
        self.nodes[0].p2p.send_message(msg_mempool())
        self.nodes[0].p2p.wait_for_disconnect()

        #mininode must be disconnected at this point
        assert_equal(len(self.nodes[0].getpeerinfo()), 0)
    
if __name__ == '__main__':
    P2PMempoolTests().main()
