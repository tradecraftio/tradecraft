#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
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
"""Test the RPC call related to the uptime command.

Test corresponds to code in rpc/server.cpp.
"""

import time

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_raises_rpc_error


class UptimeTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self._test_negative_time()
        self._test_uptime()

    def _test_negative_time(self):
        assert_raises_rpc_error(-8, "Mocktime cannot be negative: -1.", self.nodes[0].setmocktime, -1)

    def _test_uptime(self):
        wait_time = 10
        self.nodes[0].setmocktime(int(time.time() + wait_time))
        assert self.nodes[0].uptime() >= wait_time


if __name__ == '__main__':
    UptimeTest().main()
