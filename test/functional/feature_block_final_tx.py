#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core Developers
# Copyright (c) 2019-2023 The Freicoin Developers
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
"""Test the block-final transaction soft-fork logic.

Connect to a single node.
Regtest lock-in with 108/144 blocks signalling.
Activation after a further 144 blocks.
"""
from codecs import encode
import time

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    JSONRPCException,
    assert_equal,
)
from test_framework.messages import (
    COutPoint,
    CTxIn,
    CTxOut,
    CTransaction,
    ser_uint256,
    uint256_from_str,
)
from test_framework.p2p import P2PDataStore
from test_framework.blocktools import (
    create_coinbase,
    create_block,
)
from test_framework.script import (
    CScript,
    OP_FALSE,
    OP_TRUE,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)

class BlockFinalTxTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-vbparams=finaltx:0:999999999999','-whitelist=127.0.0.1', '-acceptnonstdtxn']]
        self.setup_clean_chain = True

    def bootstrap_p2p(self):
        """Add a P2P connection to the node.

        Helper to connect and wait for version handshake."""
        peer = self.nodes[0].add_p2p_connection(P2PDataStore())
        # We need to wait for the initial getheaders from the peer before we
        # start populating our blockstore. If we don't, then we may run ahead
        # to the next subtest before we receive the getheaders. We'd then send
        # an INV for the next block and receive two getheaders - one for the
        # IBD and one for the INV. We'd respond to both and could get
        # unexpectedly disconnected if the DoS score for that error is 50.
        peer.wait_for_getheaders(timeout=5)

    def generate_blocks(self, number, version, test_blocks = [], finaltx = False, sync = True):
        for i in range(number):
            block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
            block.nVersion = version
            tx_final = CTransaction()
            if finaltx:
                tx_final.nVersion = 2 # FIXME?
                tx_final.vin.extend(self.finaltx_vin)
                tx_final.vout.append(CTxOut(0, CScript([OP_TRUE])))
                tx_final.nLockTime = block.vtx[0].nLockTime
                tx_final.lock_height = block.vtx[0].lock_height
                tx_final.rehash()
                block.vtx.append(tx_final)
                block.hashMerkleRoot = block.calc_merkle_root()
            block.rehash()
            block.solve()
            if sync:
                self.nodes[0].submitblock(block.serialize().hex())
                assert_equal(self.nodes[0].getbestblockhash(), block.hash)
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            self.height += 1
            self.finaltx_vin = []
            for txout in tx_final.vout:
                self.finaltx_vin.append(CTxIn(COutPoint(tx_final.sha256, 0), CScript([]), 0xffffffff))
        return test_blocks

    def get_bip9_status(self, key):
        info = self.nodes[0].getdeploymentinfo()
        if key in info['deployments']:
            return info['deployments'][key]['bip9']
        else:
            raise KeyError(repr(info['deployments']))
            return info['deployments']['!' + key]['bip9']

    def run_test(self):
        bitno = 1
        activated_version = 0x20000000 | (1 << bitno)

        node = self.nodes[0]  # convenience reference to the node
        wallet = MiniWallet(node, mode=MiniWalletMode.RAW_OP_TRUE)

        self.bootstrap_p2p()  # Add one p2p connection to the node

        assert_equal(self.get_bip9_status('finaltx')['status'], 'defined')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'defined')
        assert_equal(self.get_bip9_status('finaltx')['since'], 0)

        self.log.info("Generate some blocks to get the chain going and un-stick the mining RPCs")
        self.generate(wallet, 2)
        assert_equal(node.getblockcount(), 2)
        self.height = 3  # height of the next block to build
        self.tip = int("0x" + node.getbestblockhash(), 0)
        self.last_block_time = int(time.time())

        self.log.info("\'finaltx\' begins in DEFINED state")
        assert_equal(self.get_bip9_status('finaltx')['status'], 'defined')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'defined')
        assert_equal(self.get_bip9_status('finaltx')['since'], 0)
        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('!finaltx' not in tmpl['rules'])
        assert('!finaltx' not in tmpl['vbavailable'])
        assert('finaltx' not in tmpl)
        assert_equal(tmpl['vbrequired'], 0)
        assert_equal(tmpl['version'] & activated_version, 0x20000000)

        self.log.info("Test 1: Advance from DEFINED to STARTED")
        test_blocks = self.generate_blocks(141, 4) # height = 143

        assert_equal(self.get_bip9_status('finaltx')['status'], 'defined')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'started')
        assert_equal(self.get_bip9_status('finaltx')['since'], 0)
        assert('statistics' not in self.get_bip9_status('finaltx'))
        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('!finaltx' not in tmpl['rules'])
        assert_equal(tmpl['vbavailable']['!finaltx'], bitno)
        assert_equal(tmpl['vbrequired'], 0)
        assert('finaltx' not in tmpl)
        assert_equal(tmpl['version'] & activated_version, activated_version)

        self.log.info("Save one of the anyone-can-spend coinbase outputs for later.")
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].nValue, 5000000000)
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].scriptPubKey, CScript([OP_TRUE]))
        early_coin = COutPoint(test_blocks[-1][0].vtx[0].sha256, 0)

        self.log.info("Test 2: Check stats after max number of \"not signalling\" blocks such that LOCKED_IN still possible this period")
        self.generate_blocks(36, 4) # 0x00000004 (not signalling)
        self.generate_blocks(10, activated_version) # 0x20000001 (not signalling)

        assert_equal(self.get_bip9_status('finaltx')['statistics']['elapsed'], 46)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['count'], 10)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['possible'], True)

        self.log.info("Test 3: Check stats after one additional \"not signalling\" block -- LOCKED_IN no longer possible this period")
        self.generate_blocks(1, 4) # 0x00000004 (not signalling)

        assert_equal(self.get_bip9_status('finaltx')['statistics']['elapsed'], 47)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['count'], 10)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['possible'], False)

        self.log.info("Test 4: Finish period with \"ready\" blocks, but soft fork will still fail to advance to LOCKED_IN")
        self.generate_blocks(97, activated_version) # 0x20000001 (signalling ready)

        assert_equal(self.get_bip9_status('finaltx')['statistics']['elapsed'], 144)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['count'], 107)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['possible'], False)
        assert_equal(self.get_bip9_status('finaltx')['status'], 'started')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'started')

        self.log.info("Test 5: Fail to achieve LOCKED_IN 100 out of 144 signal bit 1 using a variety of bits to simulate multiple parallel softforks")
        self.generate_blocks(50, activated_version) # 0x20000001 (signalling ready)
        self.generate_blocks(20, 4) # 0x00000004 (not signalling)
        self.generate_blocks(50, activated_version) # 0x20000101 (signalling ready)
        self.generate_blocks(24, 4) # 0x20010000 (not signalling)

        assert_equal(self.get_bip9_status('finaltx')['status'], 'started')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'started')
        assert_equal(self.get_bip9_status('finaltx')['since'], 144)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['elapsed'], 144)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['count'], 100)
        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('!finaltx' not in tmpl['rules'])
        assert_equal(tmpl['vbavailable']['!finaltx'], bitno)
        assert_equal(tmpl['vbrequired'], 0)
        assert_equal(tmpl['version'] & activated_version, activated_version)

        self.log.info("Test 6: 108 out of 144 signal bit 1 to achieve LOCKED_IN using a variety of bits to simulate multiple parallel softforks")
        self.generate_blocks(57, activated_version) # 0x20000001 (signalling ready)
        self.generate_blocks(26, 4) # 0x00000004 (not signalling)
        self.generate_blocks(50, activated_version) # 0x20000101 (signalling ready)
        self.generate_blocks(10, 4) # 0x20010000 (not signalling)

        self.log.info("check counting stats and \"possible\" flag before last block of this period achieves LOCKED_IN...")
        assert_equal(self.get_bip9_status('finaltx')['statistics']['elapsed'], 143)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['count'], 107)
        assert_equal(self.get_bip9_status('finaltx')['statistics']['possible'], True)
        assert_equal(self.get_bip9_status('finaltx')['status'], 'started')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'started')

        self.log.info("Test 7: ...continue with Test 6")
        self.generate_blocks(1, activated_version) # 0x20000001 (signalling ready)

        assert_equal(self.get_bip9_status('finaltx')['status'], 'started')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'locked_in')
        assert_equal(self.get_bip9_status('finaltx')['since'], 144)
        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('!finaltx' not in tmpl['rules'])

        self.log.info("Test 8: 143 more version 536870913 blocks (waiting period-1)")
        self.generate_blocks(143, 4)

        assert_equal(self.get_bip9_status('finaltx')['status'], 'locked_in')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'locked_in')
        assert_equal(self.get_bip9_status('finaltx')['since'], 576)
        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('!finaltx' not in tmpl['rules'])
        assert('!finaltx' in tmpl['vbavailable'])
        assert_equal(tmpl['vbrequired'], 0)
        assert_equal(tmpl['version'] & activated_version, activated_version)

        self.log.info("Test 9: Generate a block without any spendable outputs, which should be allowed under normal circumstances.")
        test_blocks = self.generate_blocks(1, 4, sync = False)
        for txout in test_blocks[-1][0].vtx[0].vout:
            txout.scriptPubKey = CScript([OP_FALSE])
        test_blocks[-1][0].vtx[0].rehash()
        test_blocks[-1][0].hashMerkleRoot = test_blocks[-1][0].calc_merkle_root()
        test_blocks[-1][0].rehash()
        test_blocks[-1][0].solve()
        node.submitblock(test_blocks[-1][0].serialize().hex())
        assert_equal(node.getbestblockhash(), test_blocks[-1][0].hash)
        self.tip = test_blocks[-1][0].sha256 # Hash has changed

        assert_equal(self.get_bip9_status('finaltx')['status'], 'locked_in')
        assert_equal(self.get_bip9_status('finaltx')['status_next'], 'active')
        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('!finaltx' in tmpl['rules'])
        assert('!finaltx' not in tmpl['vbavailable'])
        assert_equal(tmpl['vbrequired'], 0)
        assert(not (tmpl['version'] & (1 << bitno)))

        self.log.info("Test 10: Attempt to do the same thing: generate a block with no spendable outputs in the coinbase. This fails because the next block needs at least one trivially spendable output to start the block-final transaction chain.")
        block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
        block.nVersion = 5
        for txout in block.vtx[0].vout:
            txout.scriptPubKey = CScript([OP_FALSE])
        block.vtx[0].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), ser_uint256(self.tip)[::-1].hex())

        self.log.info("Test 11: Generate the first block with block-final-tx rules enforced, which reuires the coinbase to have a trivially-spendable output.")
        self.generate_blocks(1, 4)
        assert(any(out.scriptPubKey==CScript([OP_TRUE]) for out in test_blocks[-1][0].vtx[0].vout))
        for n,txout in enumerate(test_blocks[-1][0].vtx[0].vout):
            non_protected_output = COutPoint(test_blocks[-1][0].vtx[0].sha256, n)
            assert_equal(txout.nValue, 312500000)
        assert_equal(self.get_bip9_status('finaltx')['status'], 'active')

        self.log.info("Test 12: Generate 98 blocks (maturity period - 2)")
        self.generate_blocks(98, 4)

        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('finaltx' not in tmpl)

        self.log.info("Test 13: Generate one more block to allow non_protected_output to mature, which causes the block-final transaction to be required in the next block.")
        self.generate_blocks(1, 4)

        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('finaltx' in tmpl)
        assert_equal(len(tmpl['finaltx']['prevout']), 1)
        assert_equal(tmpl['finaltx']['prevout'][0]['txid'], encode(ser_uint256(non_protected_output.hash)[::-1], 'hex_codec').decode('ascii'))
        assert_equal(tmpl['finaltx']['prevout'][0]['vout'], non_protected_output.n)
        assert_equal(tmpl['finaltx']['prevout'][0]['amount'], 312470199)

        self.log.info("Extra pass-through value is not included in the coinbasevalue field.")
        assert_equal(tmpl['coinbasevalue'], 5000000000 // 2**(self.height // 150))

        self.log.info("The transactions field does not contain the block-final transaction.")
        assert_equal(len(tmpl['transactions']), 0)

        self.log.info("Test 14: Attempt to create a block without the block-final transaction, which fails.")
        block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
        block.nVersion = 4
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), ser_uint256(self.tip)[::-1].hex())

        self.log.info("Test 15: Add the block-final transaction, and it passes.")
        tx_final = CTransaction()
        tx_final.nVersion = 2
        tx_final.vin.append(CTxIn(non_protected_output, CScript([]), 0xffffffff))
        tx_final.vout.append(CTxOut(312470199, CScript([OP_TRUE])))
        tx_final.nLockTime = block.vtx[0].nLockTime
        tx_final.lock_height = block.vtx[0].lock_height
        tx_final.rehash()
        block.vtx.append(tx_final)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), block.hash)
        prev_final_tx = block.vtx[-1]
        self.last_block_time += 1
        self.tip = block.sha256
        self.height += 1

        tmpl = node.getblocktemplate({'rules':['segwit','finaltx']})
        assert('finaltx' in tmpl)
        assert_equal(len(tmpl['finaltx']['prevout']), 1)
        assert_equal(tmpl['finaltx']['prevout'][0]['txid'], encode(ser_uint256(tx_final.sha256)[::-1], 'hex_codec').decode('ascii'))
        assert_equal(tmpl['finaltx']['prevout'][0]['vout'], 0)
        assert_equal(tmpl['finaltx']['prevout'][0]['amount'], 312469901)

        self.log.info("Test 16: Create a block-final transaction with multiple outputs, which doesn't work because the number of outputs is restricted to be no greater than the number of inputs.")
        block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
        block.nVersion = 4
        tx_final = CTransaction()
        tx_final.nVersion = 2
        tx_final.vin.append(CTxIn(COutPoint(prev_final_tx.sha256, 0), CScript([]), 0xffffffff))
        tx_final.vout.append(CTxOut(156234951, CScript([OP_TRUE])))
        tx_final.vout.append(CTxOut(156234950, CScript([OP_TRUE])))
        tx_final.nLockTime = block.vtx[0].nLockTime
        tx_final.lock_height = block.vtx[0].lock_height
        tx_final.rehash()
        block.vtx.append(tx_final)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), ser_uint256(self.tip)[::-1].hex())

        self.log.info("Test 17: Try increasing the number of inputs by using an old one doesn't work, because the block-final transaction must source its user inputs from the same block.")
        utxo = node.gettxout(encode(ser_uint256(early_coin.hash)[::-1], 'hex_codec').decode('ascii'), early_coin.n)
        assert('amount' in utxo)
        utxo_amount = int(100000000 * utxo['amount'])
        block.vtx[-1].vin.append(CTxIn(early_coin, CScript([]), 0xffffffff))
        block.vtx[-1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), ser_uint256(self.tip)[::-1].hex())

        self.log.info("Test 18: But spend it via a user transaction instead, and it can be captured and sent to the coinbase as fee.")

        # Insert spending transaction
        spend_tx = CTransaction()
        spend_tx.nVersion = 2
        spend_tx.vin.append(CTxIn(early_coin, CScript([]), 0xffffffff))
        spend_tx.vout.append(CTxOut(utxo_amount, CScript([OP_TRUE])))
        spend_tx.nLockTime = 0
        spend_tx.lock_height = block.vtx[0].lock_height
        spend_tx.rehash()
        block.vtx.insert(1, spend_tx)
        # Capture output of spend_tx in block-final tx (but don't update the
        # outputs--the value passes on to the coinbase as fee).
        block.vtx[-1].vin[-1].prevout = COutPoint(spend_tx.sha256, 0)
        block.vtx[-1].rehash()
        # Add the captured value to the block reward.
        block.vtx[0].vout[0].nValue += utxo_amount
        block.vtx[0].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), block.hash)
        prev_final_tx = block.vtx[-1]
        self.last_block_time += 1
        self.tip = block.sha256
        self.height += 1

        self.log.info("Test 19: Spending only one of the prior outputs is insufficient.  ALL prior block-final outputs must be spent.")
        block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
        block.nVersion = 4
        tx_final = CTransaction()
        tx_final.nVersion = 2
        tx_final.vin.append(CTxIn(COutPoint(prev_final_tx.sha256, 0), CScript([]), 0xffffffff))
        tx_final.vout.append(CTxOut(156234801, CScript([OP_TRUE])))
        tx_final.nLockTime = block.vtx[0].nLockTime
        tx_final.lock_height = block.vtx[0].lock_height
        tx_final.rehash()
        block.vtx.append(tx_final)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), ser_uint256(self.tip)[::-1].hex())

        self.log.info("Test 20: But spend all the prior outputs and it goes through.")
        block.vtx[-1].vin.append(CTxIn(COutPoint(prev_final_tx.sha256, 1), CScript([]), 0xffffffff))
        block.vtx[-1].vout[0].nValue *= 2
        block.vtx[-1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), block.hash)
        prev_final_tx = block.vtx[-1]
        self.last_block_time += 1
        self.tip = block.sha256
        self.height += 1

        self.log.info("Test 21: Now that the rules have activated, transactions spending the previous block-final transaction's outputs are rejected from the mempool.")
        self.log.info("First we do this with a non-block-final input to demonstrate the test would otherwise work.")
        height = node.getblockcount() - 99
        while True:
            blk = node.getblock(node.getblockhash(height))
            txid = blk['tx'][0]
            utxo = node.gettxout(txid, 0)
            if utxo is not None and utxo['scriptPubKey']['hex'] == "51":
                break
            height -= 1

        spend_tx = CTransaction()
        spend_tx.nVersion = 2
        spend_tx.vin.append(CTxIn(COutPoint(uint256_from_str(bytes.fromhex(txid)[::-1]), 0), CScript([]), 0xffffffff))
        spend_tx.vout.append(CTxOut(int(utxo['amount']*100000000) - 10000, CScript([b'a'*100]))) # Make transaction large enough to avoid tx-size-small standardness check
        spend_tx.nLockTime = 0
        spend_tx.lock_height = utxo['refheight']
        spend_tx.rehash()
        node.sendrawtransaction(spend_tx.serialize().hex())
        mempool = node.getrawmempool()
        assert(spend_tx.hash in mempool)

        self.log.info("Now we do the same exact thing with the last block-final transaction's outputs. It should not enter the mempool.")
        spend_tx = CTransaction()
        spend_tx.nVersion = 2
        spend_tx.vin.append(CTxIn(COutPoint(prev_final_tx.sha256, 0), CScript([]), 0xffffffff))
        spend_tx.vout.append(CTxOut(int(utxo['amount']*100000000), CScript([b'a'*100]))) # Make transaction large enough to avoid tx-size-small standardness check
        spend_tx.nLockTime = 0
        spend_tx.lock_height = utxo['refheight']
        spend_tx.rehash()
        try:
            node.sendrawtransaction(spend_tx.serialize().hex())
        except JSONRPCException as e:
            assert("spend-block-final-txn" in e.error['message'])
        else:
            assert(False)
        mempool = node.getrawmempool()
        assert(spend_tx.hash not in mempool)

        self.log.info("Test 22: Invalidate the tip, then malleate and re-solve the same block. This is a fast way of testing test that the block-final txid is restored on a reorg.")
        height = node.getblockcount()
        node.invalidateblock(block.hash)
        assert_equal(node.getblockcount(), height-1)
        block.nVersion ^= 2
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getblockcount(), height)
        assert_equal(node.getbestblockhash(), block.hash)

        self.tip = block.sha256
        self.finaltx_vin = [CTxIn(COutPoint(block.vtx[-1].sha256, 0), CScript([]), 0xffffffff)]

        self.log.info("Test 22-25: Mine two blocks with trivially-spendable coinbase outputs, then test that the one that is exactly 100 blocks old is allowed to be spent in a block-final transaction, but the older one cannot.")
        self.generate_blocks(1, 4, finaltx=True)
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].scriptPubKey, CScript([OP_TRUE]))
        txin1 = CTxIn(COutPoint(test_blocks[-1][0].vtx[0].sha256, 0), CScript([]), 0xffffffff)

        self.generate_blocks(1, 4, finaltx=True)
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].scriptPubKey, CScript([OP_TRUE]))
        txin2 = CTxIn(COutPoint(test_blocks[-1][0].vtx[0].sha256, 0), CScript([]), 0xffffffff)

        self.generate_blocks(1, 4, finaltx=True)
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].scriptPubKey, CScript([OP_TRUE]))
        txin3 = CTxIn(COutPoint(test_blocks[-1][0].vtx[0].sha256, 0), CScript([]), 0xffffffff)

        self.generate_blocks(98, 4, finaltx=True)

        # txin1 is too old -- it should have been collected on the last block
        block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
        block.nVersion = 4
        tx_final = CTransaction()
        tx_final.nVersion = 2
        tx_final.vin.extend(self.finaltx_vin)
        tx_final.vin.append(txin1)
        tx_final.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx_final.nLockTime = block.vtx[0].nLockTime
        tx_final.lock_height = block.vtx[0].lock_height
        tx_final.rehash()
        block.vtx.append(tx_final)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), ser_uint256(self.tip)[::-1].hex())

        # txin3 is too young -- it hasn't matured
        block.vtx[-1].vin.pop()
        block.vtx[-1].vin.append(txin3)
        block.vtx[-1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), ser_uint256(self.tip)[::-1].hex())

        # txin2 is just right
        block.vtx[-1].vin.pop()
        block.vtx[-1].vin.append(txin2)
        block.vtx[-1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getbestblockhash(), block.hash)

        self.last_block_time += 1
        self.tip = block.sha256
        self.height += 1
        self.finaltx_vin = [CTxIn(COutPoint(block.vtx[-1].sha256, 0), CScript([]), 0xffffffff)]

if __name__ == '__main__':
    BlockFinalTxTest().main()
