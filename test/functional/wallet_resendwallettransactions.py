#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Copyright (c) 2010-2021 The Freicoin Developers
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
"""Test resendwallettransactions RPC."""

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class ResendWalletTransactionsTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['--walletbroadcast=false']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Should raise RPC_WALLET_ERROR (-4) if walletbroadcast is disabled.
        assert_raises_rpc_error(-4, "Error: Wallet transaction broadcasting is disabled with -walletbroadcast", self.nodes[0].resendwallettransactions)

        # Should return an empty array if there aren't unconfirmed wallet transactions.
        self.stop_node(0)
        self.start_node(0, extra_args=[])
        assert_equal(self.nodes[0].resendwallettransactions(), [])

        # Should return an array with the unconfirmed wallet transaction.
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        assert_equal(self.nodes[0].resendwallettransactions(), [txid])

if __name__ == '__main__':
    ResendWalletTransactionsTest().main()
