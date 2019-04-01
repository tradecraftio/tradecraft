#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
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

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


class SignMessagesTest(BitcoinTestFramework):
    """Tests RPC commands for signing and verifying messages."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        message = 'This is just a test message'

        # Test the signing with a privkey
        privKey = 'cUeKHd5orzT3mz8P9pxyREHfsWtVfgsfDjiZZBcjUBAaGk1BTj7N'
        address = 'mpLQjfK79b7CCV4VMJWEWAj5Mpx8Up5zxB'
        signature = self.nodes[0].signmessagewithprivkey(privKey, message)

        # Verify the message
        assert(self.nodes[0].verifymessage(address, signature, message))

        # Test the signing with an address with wallet
        address = self.nodes[0].getnewaddress()
        signature = self.nodes[0].signmessage(address, message)

        # Verify the message
        assert(self.nodes[0].verifymessage(address, signature, message))

if __name__ == '__main__':
    SignMessagesTest().main()
