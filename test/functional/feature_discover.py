#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
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
"""Test -discover command."""

import socket

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal


def is_valid_ipv4_address(address):
    try:
        socket.inet_aton(address)
    except socket.error:
        return False
    return True


def is_valid_ipv6_address(address):
    try:
        socket.inet_pton(socket.AF_INET6, address)
    except socket.error:
        return False
    return True


class DiscoverTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        self.num_nodes = 1

    def validate_addresses(self, addresses_obj):
        for address_obj in addresses_obj:
            address = address_obj['address']
            self.log.info(f"Validating {address}")
            valid = (is_valid_ipv4_address(address)
                     or is_valid_ipv6_address(address))
            assert_equal(valid, True)

    def test_local_addresses(self, test_case, *, expect_empty=False):
        self.log.info(f"Restart node with {test_case}")
        self.restart_node(0, test_case)
        network_info = self.nodes[0].getnetworkinfo()
        network_enabled = [n for n in network_info['networks']
                           if n['reachable'] and n['name'] in ['ipv4', 'ipv6']]
        local_addrs = list(network_info["localaddresses"])
        if expect_empty or not network_enabled:
            assert_equal(local_addrs, [])
        elif len(local_addrs) > 0:
            self.validate_addresses(local_addrs)

    def run_test(self):
        test_cases = [
            ["-listen", "-discover"],
            ["-discover"],
        ]

        test_cases_empty = [
            ["-discover=0"],
            ["-listen", "-discover=0"],
            [],
        ]

        for test_case in test_cases:
            self.test_local_addresses(test_case, expect_empty=False)

        for test_case in test_cases_empty:
            self.test_local_addresses(test_case, expect_empty=True)


if __name__ == '__main__':
    DiscoverTest().main()
