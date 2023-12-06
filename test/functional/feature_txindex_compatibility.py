#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
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
"""Test that legacy txindex will be disabled on upgrade.

Previous releases are required by this test, see test/README.md.
"""

import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error
from test_framework.wallet import MiniWallet


class TxindexCompatibilityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            ["-reindex", "-txindex"],
            [],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.add_nodes(
            self.num_nodes,
            self.extra_args,
            versions=[
                160300,  # Last release with legacy txindex
                None,  # For MiniWallet, without migration code
            ],
        )
        self.start_nodes()
        self.connect_nodes(0, 1)

    def run_test(self):
        mini_wallet = MiniWallet(self.nodes[1])
        spend_utxo = mini_wallet.get_utxo()
        mini_wallet.send_self_transfer(from_node=self.nodes[1], utxo_to_spend=spend_utxo)
        self.generate(self.nodes[1], 1)

        self.log.info("Check legacy txindex")
        assert_raises_rpc_error(-5, "Use -txindex", lambda: self.nodes[1].getrawtransaction(txid=spend_utxo["txid"]))
        self.nodes[0].getrawtransaction(txid=spend_utxo["txid"])  # Requires -txindex

        self.stop_nodes()
        legacy_chain_dir = self.nodes[0].chain_path

        self.log.info("Drop legacy txindex")
        drop_index_chain_dir = self.nodes[1].chain_path
        shutil.rmtree(drop_index_chain_dir)
        shutil.copytree(legacy_chain_dir, drop_index_chain_dir)
        # Build txindex from scratch and check there is no error this time
        self.start_node(1, extra_args=["-txindex"])
        self.wait_until(lambda: self.nodes[1].getindexinfo()["txindex"]["synced"] == True)
        self.nodes[1].getrawtransaction(txid=spend_utxo["txid"])  # Requires -txindex

        self.stop_nodes()


if __name__ == "__main__":
    TxindexCompatibilityTest().main()
