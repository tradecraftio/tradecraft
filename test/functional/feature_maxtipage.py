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
"""Test logic for setting -maxtipage on command line.

Nodes don't consider themselves out of "initial block download" as long as
their best known block header time is more than -maxtipage in the past.
"""

import time

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal


DEFAULT_MAX_TIP_AGE = 14 * 24 * 60 * 60


class MaxTipAgeTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def test_maxtipage(self, maxtipage, set_parameter=True, test_deltas=True):
        node_miner = self.nodes[0]
        node_ibd = self.nodes[1]

        self.restart_node(1, [f'-maxtipage={maxtipage}'] if set_parameter else None)
        self.connect_nodes(0, 1)
        cur_time = int(time.time())

        if test_deltas:
            # tips older than maximum age -> stay in IBD
            node_ibd.setmocktime(cur_time)
            for delta in [5, 4, 3, 2, 1]:
                node_miner.setmocktime(cur_time - maxtipage - delta)
                self.generate(node_miner, 1)
                assert_equal(node_ibd.getblockchaininfo()['initialblockdownload'], True)

        # tip within maximum age -> leave IBD
        node_miner.setmocktime(max(cur_time - maxtipage, 0))
        self.generate(node_miner, 1)
        assert_equal(node_ibd.getblockchaininfo()['initialblockdownload'], False)

    def run_test(self):
        self.log.info("Test IBD with maximum tip age of 14 days (default).")
        self.test_maxtipage(DEFAULT_MAX_TIP_AGE, set_parameter=False)

        for hours in [20, 10, 5, 2, 1]:
            maxtipage = hours * 60 * 60
            self.log.info(f"Test IBD with maximum tip age of {hours} hours (-maxtipage={maxtipage}).")
            self.test_maxtipage(maxtipage)

        max_long_val = 9223372036854775807
        self.log.info(f"Test IBD with highest allowable maximum tip age ({max_long_val}).")
        self.test_maxtipage(max_long_val, test_deltas=False)


if __name__ == '__main__':
    MaxTipAgeTest().main()
