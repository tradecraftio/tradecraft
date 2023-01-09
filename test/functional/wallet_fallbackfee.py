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
"""Test wallet replace-by-fee capabilities in conjunction with the fallbackfee."""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_raises_rpc_error

class WalletRBFTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)

        # sending a transaction without fee estimations must be possible by default on regtest
        self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)

        # test sending a tx with disabled fallback fee (must fail)
        self.restart_node(0, extra_args=["-fallbackfee=0"])
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1))
        assert_raises_rpc_error(-4, "Fee estimation failed", lambda: self.nodes[0].fundrawtransaction(self.nodes[0].createrawtransaction([], {self.nodes[0].getnewaddress(): 1})))
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: self.nodes[0].sendmany("", {self.nodes[0].getnewaddress(): 1}))

if __name__ == '__main__':
    WalletRBFTest().main()
