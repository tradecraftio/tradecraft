#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
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

import os

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_raises_rpc_error

class WalletCrossChain(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.add_nodes(self.num_nodes)

        # Switch node 1 to testnet before starting it.
        self.nodes[1].chain = 'testnet3'
        self.nodes[1].extra_args = ['-maxconnections=0'] # disable testnet sync
        with open(self.nodes[1].freicoinconf, 'r', encoding='utf8') as conf:
            conf_data = conf.read()
        with open (self.nodes[1].freicoinconf, 'w', encoding='utf8') as conf:
            conf.write(conf_data.replace('regtest=', 'testnet=').replace('[regtest]', '[test]'))

        self.start_nodes()

    def run_test(self):
        self.log.info("Creating wallets")

        node0_wallet = os.path.join(self.nodes[0].datadir, 'node0_wallet')
        self.nodes[0].createwallet(node0_wallet)
        self.nodes[0].unloadwallet(node0_wallet)
        node1_wallet = os.path.join(self.nodes[1].datadir, 'node1_wallet')
        self.nodes[1].createwallet(node1_wallet)
        self.nodes[1].unloadwallet(node1_wallet)

        self.log.info("Loading wallets into nodes with a different genesis blocks")

        if self.options.descriptors:
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[0].loadwallet, node1_wallet)
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[1].loadwallet, node0_wallet)
        else:
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[0].loadwallet, node1_wallet)
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[1].loadwallet, node0_wallet)

        if not self.options.descriptors:
            self.log.info("Override cross-chain wallet load protection")
            self.stop_nodes()
            self.start_nodes([['-walletcrosschain']] * self.num_nodes)
            self.nodes[0].loadwallet(node1_wallet)
            self.nodes[1].loadwallet(node0_wallet)


if __name__ == '__main__':
    WalletCrossChain().main()
