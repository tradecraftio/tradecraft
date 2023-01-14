#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
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

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal


class WalletLocktimeTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]

        mtp_tip = node.getblockheader(node.getbestblockhash())["mediantime"]

        self.log.info("Get new address with label")
        label = "timelock⌛🔓"
        address = node.getnewaddress(label=label)

        self.log.info("Send to new address with locktime")
        node.send(
            outputs={address: 5},
            options={"locktime": mtp_tip - 1},
        )
        self.generate(node, 1)

        self.log.info("Check that clock cannot change finality of confirmed txs")
        amount_before_ad = node.getreceivedbyaddress(address)
        amount_before_lb = node.getreceivedbylabel(label)
        list_before_ad = node.listreceivedbyaddress(address_filter=address)
        list_before_lb = node.listreceivedbylabel(include_empty=False)
        balance_before = node.getbalances()["mine"]["trusted"]
        coin_before = node.listunspent(maxconf=1)
        node.setmocktime(mtp_tip - 1)
        assert_equal(node.getreceivedbyaddress(address), amount_before_ad)
        assert_equal(node.getreceivedbylabel(label), amount_before_lb)
        assert_equal(node.listreceivedbyaddress(address_filter=address), list_before_ad)
        assert_equal(node.listreceivedbylabel(include_empty=False), list_before_lb)
        assert_equal(node.getbalances()["mine"]["trusted"], balance_before)
        assert_equal(node.listunspent(maxconf=1), coin_before)


if __name__ == "__main__":
    WalletLocktimeTest().main()
