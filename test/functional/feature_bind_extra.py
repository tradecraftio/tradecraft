#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
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
"""
Test starting freicoind with -bind and/or -bind=...=onion and confirm
that bind happens on the expected ports.
"""

import sys

from test_framework.netutil import (
    addr_to_hex,
    get_bind_addrs,
)
from test_framework.test_framework import (
    FreicoinTestFramework,
    SkipTest,
)
from test_framework.util import (
    assert_equal,
    p2p_port,
    rpc_port,
)


class BindExtraTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # Avoid any -bind= on the command line. Force the framework to avoid
        # adding -bind=127.0.0.1.
        self.bind_to_localhost_only = False
        self.num_nodes = 2

    def setup_network(self):
        # Due to OS-specific network stats queries, we only run on Linux.
        self.log.info("Checking for Linux")
        if not sys.platform.startswith('linux'):
            raise SkipTest("This test can only be run on Linux.")

        loopback_ipv4 = addr_to_hex("127.0.0.1")

        # Start custom ports by reusing unused p2p ports
        port = p2p_port(self.num_nodes)

        # Array of tuples [command line arguments, expected bind addresses].
        self.expected = []

        # Node0, no normal -bind=... with -bind=...=onion, thus only the tor target.
        self.expected.append(
            [
                [f"-bind=127.0.0.1:{port}=onion"],
                [(loopback_ipv4, port)]
            ],
        )
        port += 1

        # Node1, both -bind=... and -bind=...=onion.
        self.expected.append(
            [
                [f"-bind=127.0.0.1:{port}", f"-bind=127.0.0.1:{port + 1}=onion"],
                [(loopback_ipv4, port), (loopback_ipv4, port + 1)]
            ],
        )
        port += 2

        self.extra_args = list(map(lambda e: e[0], self.expected))
        self.add_nodes(self.num_nodes, self.extra_args)
        # Don't start the nodes, as some of them would collide trying to bind on the same port.

    def run_test(self):
        for i in range(len(self.expected)):
            self.log.info(f"Starting node {i} with {self.expected[i][0]}")
            self.start_node(i)
            pid = self.nodes[i].process.pid
            binds = set(get_bind_addrs(pid))
            # Remove IPv6 addresses because on some CI environments "::1" is not configured
            # on the system (so our test_ipv6_local() would return False), but it is
            # possible to bind on "::". This makes it unpredictable whether to expect
            # that freicoind has bound on "::1" (for RPC) and "::" (for P2P).
            ipv6_addr_len_bytes = 32
            binds = set(filter(lambda e: len(e[0]) != ipv6_addr_len_bytes, binds))
            # Remove RPC ports. They are not relevant for this test.
            binds = set(filter(lambda e: e[1] != rpc_port(i), binds))
            assert_equal(binds, set(self.expected[i][1]))
            self.stop_node(i)
            self.log.info(f"Stopped node {i}")

if __name__ == '__main__':
    BindExtraTest().main()
