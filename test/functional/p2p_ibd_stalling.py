#!/usr/bin/env python3
# Copyright (c) 2022- The Bitcoin Core developers
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
"""
Test stalling logic during IBD
"""

import time

from test_framework.blocktools import (
        create_block,
        create_coinbase,
        add_final_tx,
)
from test_framework.messages import (
        CTxOut,
        MSG_BLOCK,
        MSG_TYPE_MASK,
)
from test_framework.p2p import (
        CBlockHeader,
        msg_block,
        msg_headers,
        P2PDataStore,
)
from test_framework.script import (
        CScript,
        OP_TRUE,
)
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
        assert_equal,
)


class P2PStaller(P2PDataStore):
    def __init__(self, stall_block):
        self.stall_block = stall_block
        super().__init__()

    def on_getdata(self, message):
        for inv in message.inv:
            self.getdata_requests.append(inv.hash)
            if (inv.type & MSG_TYPE_MASK) == MSG_BLOCK:
                if (inv.hash != self.stall_block):
                    self.send_message(msg_block(self.block_store[inv.hash]))

    def on_getheaders(self, message):
        pass


class P2PIBDStallingTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        NUM_BLOCKS = 1025
        NUM_PEERS = 4
        node = self.nodes[0]
        tip = int(node.getbestblockhash(), 16)
        blocks = []
        height = 1
        block_time = node.getblock(node.getbestblockhash())['time'] + 1
        self.log.info("Prepare blocks without sending them to the node")
        block_dict = {}
        for _ in range(NUM_BLOCKS):
            block = create_block(tip, create_coinbase(height), block_time)
            if height == 1:
                block.vtx[0].vout.insert(0, CTxOut(0, CScript([OP_TRUE])))
                block.vtx[0].rehash()
                final_tx = [{
                    'txid': block.vtx[0].hash,
                    'vout': 0,
                    'amount': 0,
                }]
                block.hashMerkleRoot = block.calc_merkle_root()
                block.rehash()
            if height > 100:
                final_tx = add_final_tx(final_tx, block)
            block.solve()
            blocks.append(block)
            tip = block.sha256
            block_time += 1
            height += 1
            block_dict[tip] = block
        stall_block = blocks[0].sha256

        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in blocks[:NUM_BLOCKS-1]]
        peers = []

        self.log.info("Check that a staller does not get disconnected if the 1024 block lookahead buffer is filled")
        for id in range(NUM_PEERS):
            peers.append(node.add_outbound_p2p_connection(P2PStaller(stall_block), p2p_idx=id, connection_type="outbound-full-relay"))
            peers[-1].block_store = block_dict
            peers[-1].send_message(headers_message)

        # Need to wait until 1023 blocks are received - the magic total bytes number is a workaround in lack of an rpc
        # returning the number of downloaded (but not connected) blocks.
        self.wait_until(lambda: self.total_bytes_recv_for_blocks() == 236913)

        self.all_sync_send_with_ping(peers)
        # If there was a peer marked for stalling, it would get disconnected
        self.mocktime = int(time.time()) + 3
        node.setmocktime(self.mocktime)
        self.all_sync_send_with_ping(peers)
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS)

        self.log.info("Check that increasing the window beyond 1024 blocks triggers stalling logic")
        headers_message.headers = [CBlockHeader(b) for b in blocks]
        with node.assert_debug_log(expected_msgs=['Stall started']):
            for p in peers:
                p.send_message(headers_message)
            self.all_sync_send_with_ping(peers)

        self.log.info("Check that the stalling peer is disconnected after 2 seconds")
        self.mocktime += 3
        node.setmocktime(self.mocktime)
        peers[0].wait_for_disconnect()
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS - 1)
        self.wait_until(lambda: self.is_block_requested(peers, stall_block))
        # Make sure that SendMessages() is invoked, which assigns the missing block
        # to another peer and starts the stalling logic for them
        self.all_sync_send_with_ping(peers)

        self.log.info("Check that the stalling timeout gets doubled to 4 seconds for the next staller")
        # No disconnect after just 3 seconds
        self.mocktime += 3
        node.setmocktime(self.mocktime)
        self.all_sync_send_with_ping(peers)
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS - 1)

        self.mocktime += 2
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(x.is_connected for x in node.p2ps) == NUM_PEERS - 2)
        self.wait_until(lambda: self.is_block_requested(peers, stall_block))
        self.all_sync_send_with_ping(peers)

        self.log.info("Check that the stalling timeout gets doubled to 8 seconds for the next staller")
        # No disconnect after just 7 seconds
        self.mocktime += 7
        node.setmocktime(self.mocktime)
        self.all_sync_send_with_ping(peers)
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS - 2)

        self.mocktime += 2
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(x.is_connected for x in node.p2ps) == NUM_PEERS - 3)
        self.wait_until(lambda: self.is_block_requested(peers, stall_block))
        self.all_sync_send_with_ping(peers)

        self.log.info("Provide the withheld block and check that stalling timeout gets reduced back to 2 seconds")
        with node.assert_debug_log(expected_msgs=['Decreased stalling timeout to 2 seconds']):
            for p in peers:
                if p.is_connected and (stall_block in p.getdata_requests):
                    p.send_message(msg_block(block_dict[stall_block]))

        self.log.info("Check that all outstanding blocks get connected")
        self.wait_until(lambda: node.getblockcount() == NUM_BLOCKS)

    def total_bytes_recv_for_blocks(self):
        total = 0
        for info in self.nodes[0].getpeerinfo():
            if ("block" in info["bytesrecv_per_msg"].keys()):
                total += info["bytesrecv_per_msg"]["block"]
        return total

    def all_sync_send_with_ping(self, peers):
        for p in peers:
            if p.is_connected:
                p.sync_send_with_ping()

    def is_block_requested(self, peers, hash):
        for p in peers:
            if p.is_connected and (hash in p.getdata_requests):
                return True
        return False


if __name__ == '__main__':
    P2PIBDStallingTest().main()