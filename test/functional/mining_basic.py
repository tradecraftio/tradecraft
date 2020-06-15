#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
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
"""Test mining RPCs

- getmininginfo
- getblocktemplate proposal mode
- submitblock"""

import copy
from decimal import Decimal

from test_framework.blocktools import (
    add_final_tx,
    create_coinbase,
    TIME_GENESIS_BLOCK,
)
from test_framework.messages import (
    CBlock,
    CBlockHeader,
    BLOCK_HEADER_SIZE
)
from test_framework.mininode import (
    P2PDataStore,
)
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    bytes_to_hex_str as b2x,
    connect_nodes_bi,
)
from test_framework.script import CScriptNum


def assert_template(node, block, expect, rehash=True):
    if rehash:
        block.hashMerkleRoot = block.calc_merkle_root()
    rsp = node.getblocktemplate(template_request={'data': b2x(block.serialize()), 'mode': 'proposal', 'rules': ['segwit']})
    assert_equal(rsp, expect)


class MiningTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def mine_chain(self):
        self.log.info('Create some old blocks')
        for t in range(TIME_GENESIS_BLOCK, TIME_GENESIS_BLOCK + 200 * 600, 600):
            self.nodes[0].setmocktime(t)
            self.nodes[0].generate(1)
        mining_info = self.nodes[0].getmininginfo()
        assert_equal(mining_info['blocks'], 200)
        assert_equal(mining_info['currentblocktx'], 0)
        assert_equal(mining_info['currentblockweight'], 4260)
        self.restart_node(0)
        connect_nodes_bi(self.nodes, 0, 1)

    def run_test(self):
        self.mine_chain()
        node = self.nodes[0]

        def assert_submitblock(block, result_str_1, result_str_2=None):
            block.solve()
            result_str_2 = result_str_2 or 'duplicate-invalid'
            assert_equal(result_str_1, node.submitblock(hexdata=b2x(block.serialize())))
            assert_equal(result_str_2, node.submitblock(hexdata=b2x(block.serialize())))

        self.log.info('getmininginfo')
        mining_info = node.getmininginfo()
        assert_equal(mining_info['blocks'], 200)
        assert_equal(mining_info['chain'], 'regtest')
        assert 'currentblocktx' not in mining_info
        assert 'currentblockweight' not in mining_info
        assert_equal(mining_info['difficulty'], Decimal('4.656542373906925E-10'))
        assert_equal(mining_info['networkhashps'], Decimal('0.003333333333333334'))
        assert_equal(mining_info['pooledtx'], 0)

        # Mine a block to leave initial block download
        node.generatetoaddress(1, node.get_deterministic_priv_key().address)
        tmpl = node.getblocktemplate({'rules': ['segwit','auxpow']})
        self.log.info("getblocktemplate: Test capability advertised")
        assert 'proposal' in tmpl['capabilities']
        assert 'coinbasetxn' not in tmpl

        next_height = int(tmpl["height"])
        coinbase_tx = create_coinbase(height=next_height)
        # sequence numbers must not be max for nLockTime to have effect
        coinbase_tx.vin[0].nSequence = 2 ** 32 - 2
        coinbase_tx.rehash()

        # round-trip the encoded bip34 block height commitment
        assert_equal(CScriptNum.decode(coinbase_tx.vin[0].scriptSig), next_height)
        # round-trip negative and multi-byte CScriptNums to catch python regression
        assert_equal(CScriptNum.decode(CScriptNum.encode(CScriptNum(1500))), 1500)
        assert_equal(CScriptNum.decode(CScriptNum.encode(CScriptNum(-1500))), -1500)
        assert_equal(CScriptNum.decode(CScriptNum.encode(CScriptNum(-1))), -1)

        block = CBlock()
        block.nVersion = tmpl["version"]
        block.hashPrevBlock = int(tmpl["previousblockhash"], 16)
        block.nTime = tmpl["curtime"]
        block.nBits = int(tmpl["bits"], 16)
        block.nNonce = 0
        block.vtx = [coinbase_tx]

        if 'finaltx' in tmpl and 'prevout' in tmpl['finaltx']:
            add_final_tx(tmpl['finaltx']['prevout'], block)

        self.log.info("getblocktemplate: segwit rule must be set")
        assert_raises_rpc_error(-8, "getblocktemplate must be called with the segwit rule set", node.getblocktemplate)

        self.log.info("getblocktemplate: Test valid block")
        assert_template(node, block, None)

        self.log.info("submitblock: Test block decode failure")
        assert_raises_rpc_error(-22, "Block decode failed", node.submitblock, b2x(block.serialize()[:-15]))

        self.log.info("getblocktemplate: Test bad input hash for coinbase transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx[0].vin[0].prevout.hash += 1
        bad_block.vtx[0].rehash()
        assert_template(node, bad_block, 'bad-cb-missing')

        self.log.info("submitblock: Test invalid coinbase transaction")
        assert_raises_rpc_error(-22, "Block does not start with a coinbase", node.submitblock, b2x(bad_block.serialize()))

        self.log.info("getblocktemplate: Test truncated final transaction")
        assert_raises_rpc_error(-22, "Block decode failed", node.getblocktemplate, {'data': b2x(block.serialize()[:-1]), 'mode': 'proposal', 'rules': ['segwit']})

        self.log.info("getblocktemplate: Test duplicate transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx.insert(-1, bad_block.vtx[0])
        assert_template(node, bad_block, 'bad-txns-duplicate')
        assert_submitblock(bad_block, 'bad-txns-duplicate', 'bad-txns-duplicate')

        self.log.info("getblocktemplate: Test invalid transaction")
        bad_block = copy.deepcopy(block)
        bad_tx = copy.deepcopy(bad_block.vtx[0])
        bad_tx.vin[0].prevout.hash = 255
        bad_tx.rehash()
        bad_block.vtx.insert(-1, bad_tx)
        assert_template(node, bad_block, 'bad-txns-inputs-missingorspent')
        assert_submitblock(bad_block, 'bad-txns-inputs-missingorspent')

        self.log.info("getblocktemplate: Test nonfinal transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx[0].nLockTime = 2 ** 32 - 1
        bad_block.vtx[0].rehash()
        assert_template(node, bad_block, 'bad-txns-nonfinal')
        assert_submitblock(bad_block, 'bad-txns-nonfinal')

        self.log.info("getblocktemplate: Test bad tx count")
        # The tx count is immediately after the block header
        bad_block_sn = bytearray(block.serialize())
        assert_equal(bad_block_sn[BLOCK_HEADER_SIZE], 2)
        bad_block_sn[BLOCK_HEADER_SIZE] += 1
        assert_raises_rpc_error(-22, "Block decode failed", node.getblocktemplate, {'data': b2x(bad_block_sn), 'mode': 'proposal', 'rules': ['segwit', 'auxpow']})

        self.log.info("getblocktemplate: Test bad bits")
        bad_block = copy.deepcopy(block)
        bad_block.nBits = 469762303  # impossible in the real world
        assert_template(node, bad_block, 'bad-diffbits')

        self.log.info("getblocktemplate: Test bad merkle root")
        bad_block = copy.deepcopy(block)
        bad_block.hashMerkleRoot += 1
        assert_template(node, bad_block, 'bad-txnmrklroot', False)
        assert_submitblock(bad_block, 'bad-txnmrklroot', 'bad-txnmrklroot')

        self.log.info("getblocktemplate: Test bad timestamps")
        bad_block = copy.deepcopy(block)
        bad_block.nTime = 2 ** 31 - 1
        assert_template(node, bad_block, 'time-too-new')
        assert_submitblock(bad_block, 'time-too-new', 'time-too-new')
        bad_block.nTime = 0
        assert_template(node, bad_block, 'time-too-old')
        assert_submitblock(bad_block, 'time-too-old', 'time-too-old')

        self.log.info("getblocktemplate: Test not best block")
        bad_block = copy.deepcopy(block)
        bad_block.hashPrevBlock = 123
        assert_template(node, bad_block, 'inconclusive-not-best-prevblk')
        assert_submitblock(bad_block, 'prev-blk-not-found', 'prev-blk-not-found')

        self.log.info('submitheader tests')
        assert_raises_rpc_error(-22, 'Block header decode failed', lambda: node.submitheader(hexdata='xx' * BLOCK_HEADER_SIZE))
        assert_raises_rpc_error(-22, 'Block header decode failed', lambda: node.submitheader(hexdata='ff' * (BLOCK_HEADER_SIZE-2)))
        assert_raises_rpc_error(-25, 'Must submit previous header', lambda: node.submitheader(hexdata=b2x(super(CBlock, bad_block).serialize())))

        block.nTime += 1
        block.solve()

        def chain_tip(b_hash, *, status='headers-only', branchlen=1):
            return {'hash': b_hash, 'height': 202, 'branchlen': branchlen, 'status': status}

        assert chain_tip(block.hash) not in node.getchaintips()
        node.submitheader(hexdata=b2x(block.serialize()))
        assert chain_tip(block.hash) in node.getchaintips()
        node.submitheader(hexdata=b2x(CBlockHeader(block).serialize()))  # Noop
        assert chain_tip(block.hash) in node.getchaintips()

        bad_block_root = copy.deepcopy(block)
        bad_block_root.hashMerkleRoot += 2
        bad_block_root.solve()
        assert chain_tip(bad_block_root.hash) not in node.getchaintips()
        node.submitheader(hexdata=b2x(CBlockHeader(bad_block_root).serialize()))
        assert chain_tip(bad_block_root.hash) in node.getchaintips()
        # Should still reject invalid blocks, even if we have the header:
        assert_equal(node.submitblock(hexdata=b2x(bad_block_root.serialize())), 'bad-txnmrklroot')
        assert_equal(node.submitblock(hexdata=b2x(bad_block_root.serialize())), 'bad-txnmrklroot')
        assert chain_tip(bad_block_root.hash) in node.getchaintips()
        # We know the header for this invalid block, so should just return early without error:
        node.submitheader(hexdata=b2x(CBlockHeader(bad_block_root).serialize()))
        assert chain_tip(bad_block_root.hash) in node.getchaintips()

        bad_block_lock = copy.deepcopy(block)
        bad_block_lock.vtx[0].nLockTime = 2**32 - 1
        bad_block_lock.vtx[0].rehash()
        bad_block_lock.hashMerkleRoot = bad_block_lock.calc_merkle_root()
        bad_block_lock.solve()
        assert_equal(node.submitblock(hexdata=b2x(bad_block_lock.serialize())), 'bad-txns-nonfinal')
        assert_equal(node.submitblock(hexdata=b2x(bad_block_lock.serialize())), 'duplicate-invalid')
        # Build a "good" block on top of the submitted bad block
        bad_block2 = copy.deepcopy(block)
        bad_block2.hashPrevBlock = bad_block_lock.sha256
        bad_block2.solve()
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(hexdata=b2x(CBlockHeader(bad_block2).serialize())))

        # Should reject invalid header right away
        bad_block_time = copy.deepcopy(block)
        bad_block_time.nTime = 1
        bad_block_time.solve()
        assert_raises_rpc_error(-25, 'time-too-old', lambda: node.submitheader(hexdata=b2x(CBlockHeader(bad_block_time).serialize())))

        # Should ask for the block from a p2p node, if they announce the header as well:
        node.add_p2p_connection(P2PDataStore())
        node.p2p.wait_for_getheaders(timeout=5)  # Drop the first getheaders
        node.p2p.send_blocks_and_test(blocks=[block], node=node)
        # Must be active now:
        assert chain_tip(block.hash, status='active', branchlen=0) in node.getchaintips()

        # Building a few blocks should give the same results
        node.generatetoaddress(10, node.get_deterministic_priv_key().address)
        assert_raises_rpc_error(-25, 'time-too-old', lambda: node.submitheader(hexdata=b2x(CBlockHeader(bad_block_time).serialize())))
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(hexdata=b2x(CBlockHeader(bad_block2).serialize())))
        node.submitheader(hexdata=b2x(CBlockHeader(block).serialize()))
        node.submitheader(hexdata=b2x(CBlockHeader(bad_block_root).serialize()))
        assert_equal(node.submitblock(hexdata=b2x(block.serialize())), 'duplicate')  # valid


if __name__ == '__main__':
    MiningTest().main()
