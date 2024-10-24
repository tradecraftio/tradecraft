#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
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
"""Test the estimatefee RPCs.

Test the following RPCs:
   - estimatesmartfee
   - estimaterawfee
"""

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_raises_rpc_error

class EstimateFeeTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        # missing required params
        assert_raises_rpc_error(-1, "estimatesmartfee", self.nodes[0].estimatesmartfee)
        assert_raises_rpc_error(-1, "estimaterawfee", self.nodes[0].estimaterawfee)

        # wrong type for conf_target
        assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].estimatesmartfee, 'foo')
        assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].estimaterawfee, 'foo')

        # wrong type for estimatesmartfee(estimate_mode)
        assert_raises_rpc_error(-3, "JSON value of type number is not of expected type string", self.nodes[0].estimatesmartfee, 1, 1)
        assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"', self.nodes[0].estimatesmartfee, 1, 'foo')

        # wrong type for estimaterawfee(threshold)
        assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].estimaterawfee, 1, 'foo')

        # extra params
        assert_raises_rpc_error(-1, "estimatesmartfee", self.nodes[0].estimatesmartfee, 1, 'ECONOMICAL', 1)
        assert_raises_rpc_error(-1, "estimaterawfee", self.nodes[0].estimaterawfee, 1, 1, 1)

        # max value of 1008 per src/policy/fees.h
        assert_raises_rpc_error(-8, "Invalid conf_target, must be between 1 and 1008", self.nodes[0].estimaterawfee, 1009)

        # valid calls
        self.nodes[0].estimatesmartfee(1)
        # self.nodes[0].estimatesmartfee(1, None)
        self.nodes[0].estimatesmartfee(1, 'ECONOMICAL')
        self.nodes[0].estimatesmartfee(1, 'unset')
        self.nodes[0].estimatesmartfee(1, 'conservative')

        self.nodes[0].estimaterawfee(1)
        self.nodes[0].estimaterawfee(1, None)
        self.nodes[0].estimaterawfee(1, 1)


if __name__ == '__main__':
    EstimateFeeTest().main()
