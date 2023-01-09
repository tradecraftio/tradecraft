#!/usr/bin/env python3
# Copyright (c) 2018-2021 The Bitcoin Core developers
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
"""Verify that starting freicoin with -h works as expected."""

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal

class HelpTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Don't start the node

    def get_node_output(self, *, ret_code_expected):
        ret_code = self.nodes[0].process.wait(timeout=60)
        assert_equal(ret_code, ret_code_expected)
        self.nodes[0].stdout.seek(0)
        self.nodes[0].stderr.seek(0)
        out = self.nodes[0].stdout.read()
        err = self.nodes[0].stderr.read()
        self.nodes[0].stdout.close()
        self.nodes[0].stderr.close()

        # Clean up TestNode state
        self.nodes[0].running = False
        self.nodes[0].process = None
        self.nodes[0].rpc_connected = False
        self.nodes[0].rpc = None

        return out, err

    def run_test(self):
        self.log.info("Start freicoin with -h for help text")
        self.nodes[0].start(extra_args=['-h'])
        # Node should exit immediately and output help to stdout.
        output, _ = self.get_node_output(ret_code_expected=0)
        assert b'Options' in output
        self.log.info(f"Help text received: {output[0:60]} (...)")

        self.log.info("Start freicoin with -version for version information")
        self.nodes[0].start(extra_args=['-version'])
        # Node should exit immediately and output version to stdout.
        output, _ = self.get_node_output(ret_code_expected=0)
        assert b'version' in output
        self.log.info(f"Version text received: {output[0:60]} (...)")

        # Test that arguments not in the help results in an error
        self.log.info("Start freicoind with -fakearg to make sure it does not start")
        self.nodes[0].start(extra_args=['-fakearg'])
        # Node should exit immediately and output an error to stderr
        _, output = self.get_node_output(ret_code_expected=1)
        assert b'Error parsing command line arguments' in output
        self.log.info(f"Error message received: {output[0:60]} (...)")


if __name__ == '__main__':
    HelpTest().main()
