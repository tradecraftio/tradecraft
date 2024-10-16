#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Bitcoin Core developers
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
Test ports handling for I2P hosts
"""


from test_framework.test_framework import FreicoinTestFramework

PROXY = "127.0.0.1:60000"

class I2PPorts(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # The test assumes that an I2P SAM proxy is not listening here.
        self.extra_args = [[f"-i2psam={PROXY}"]]

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Ensure we don't try to connect if port!=0")
        addr = "zsxwyo6qcn3chqzwxnseusqgsnuw3maqnztkiypyfxtya4snkoka.b32.i2p:8333"
        with node.assert_debug_log(expected_msgs=[f"Error connecting to {addr}, connection refused due to arbitrary port 8333"]):
            node.addnode(node=addr, command="onetry")

        self.log.info("Ensure we try to connect if port=0 and get an error due to missing I2P proxy")
        addr = "h3r6bkn46qxftwja53pxiykntegfyfjqtnzbm6iv6r5mungmqgmq.b32.i2p:0"
        with node.assert_debug_log(expected_msgs=[f"Error connecting to {addr}: Cannot connect to {PROXY}"]):
            node.addnode(node=addr, command="onetry")


if __name__ == '__main__':
    I2PPorts(__file__).main()
