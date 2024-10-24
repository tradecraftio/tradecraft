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
"""
Test that transaction announcements are only queued for peers that have
successfully completed the version handshake.

Topology:

  tx_originator ----> node[0] <---- spy

We test that a transaction sent by tx_originator is only relayed to spy
if it was received after spy's version handshake completed.

1. Fully connect tx_originator
2. Connect spy (no version handshake)
3. tx_originator sends tx1
4. spy completes the version handshake
5. tx_originator sends tx2
6. We check that only tx2 is announced on the spy interface
"""
from test_framework.messages import (
    msg_wtxidrelay,
    msg_verack,
    msg_tx,
    CInv,
    MSG_WTX,
)
from test_framework.p2p import (
    P2PInterface,
)
from test_framework.test_framework import FreicoinTestFramework
from test_framework.wallet import MiniWallet

class P2PTxSpy(P2PInterface):
    def __init__(self):
        super().__init__()
        self.all_invs = []

    def on_version(self, message):
        self.send_message(msg_wtxidrelay())

    def on_inv(self, message):
        self.all_invs += message.inv

    def wait_for_inv_match(self, expected_inv):
        self.wait_until(lambda: len(self.all_invs) == 1 and self.all_invs[0] == expected_inv)

class TxPrivacyTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        tx_originator = self.nodes[0].add_p2p_connection(P2PInterface())
        spy = self.nodes[0].add_p2p_connection(P2PTxSpy(), wait_for_verack=False)
        spy.wait_for_verack()

        # tx_originator sends tx1
        tx1 = self.wallet.create_self_transfer()["tx"]
        tx_originator.send_and_ping(msg_tx(tx1))

        # Spy sends the verack
        spy.send_and_ping(msg_verack())

        # tx_originator sends tx2
        tx2 = self.wallet.create_self_transfer()["tx"]
        tx_originator.send_and_ping(msg_tx(tx2))

        # Spy should only get an inv for the second transaction as the first
        # one was received pre-verack with the spy
        spy.wait_for_inv_match(CInv(MSG_WTX, tx2.calc_sha256(True)))

if __name__ == '__main__':
    TxPrivacyTest().main()
