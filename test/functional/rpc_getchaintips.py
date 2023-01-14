#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
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
"""Test the getchaintips RPC.

- introduce a network split
- work on chains of different lengths
- join the network together again
- verify that getchaintips now returns two chain tips.
"""

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal

class GetChainTipsTest (FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4

    def run_test(self):
        tips = self.nodes[0].getchaintips()
        assert_equal(len(tips), 1)
        assert_equal(tips[0]['branchlen'], 0)
        assert_equal(tips[0]['height'], 200)
        assert_equal(tips[0]['status'], 'active')

        # Split the network and build two chains of different lengths.
        self.split_network()
        self.generate(self.nodes[0], 10, sync_fun=lambda: self.sync_all(self.nodes[:2]))
        self.generate(self.nodes[2], 20, sync_fun=lambda: self.sync_all(self.nodes[2:]))

        tips = self.nodes[1].getchaintips ()
        assert_equal (len (tips), 1)
        shortTip = tips[0]
        assert_equal (shortTip['branchlen'], 0)
        assert_equal (shortTip['height'], 210)
        assert_equal (tips[0]['status'], 'active')

        tips = self.nodes[3].getchaintips ()
        assert_equal (len (tips), 1)
        longTip = tips[0]
        assert_equal (longTip['branchlen'], 0)
        assert_equal (longTip['height'], 220)
        assert_equal (tips[0]['status'], 'active')

        # Join the network halves and check that we now have two tips
        # (at least at the nodes that previously had the short chain).
        self.join_network ()

        tips = self.nodes[0].getchaintips ()
        assert_equal (len (tips), 2)
        assert_equal (tips[0], longTip)

        assert_equal (tips[1]['branchlen'], 10)
        assert_equal (tips[1]['status'], 'valid-fork')
        tips[1]['branchlen'] = 0
        tips[1]['status'] = 'active'
        assert_equal (tips[1], shortTip)

if __name__ == '__main__':
    GetChainTipsTest ().main ()
