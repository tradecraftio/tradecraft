#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core Developers
# Copyright (c) 2019-2021 The Freicoin Developers
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
from io import BytesIO
import shutil
import time
import itertools

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.mininode import COutPoint, CTxIn, CTxOut, CTransaction, NetworkThread, ser_uint256, uint256_from_str, ToHex
from test_framework.blocktools import create_coinbase, create_block
from test_framework.comptool import TestInstance, TestManager
from test_framework.script import CScript, OP_1NEGATE, OP_CHECKSEQUENCEVERIFY, OP_DROP, OP_FALSE, OP_TRUE

class BlockFinalTxTest(ComparisonTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-vbparams=finaltx:0:999999999999',
                            '-whitelist=127.0.0.1']]
        self.setup_clean_chain = True

    def run_test(self):
        self.test = TestManager(self, self.options.tmpdir)
        self.test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        self.test.run()

    def generate_blocks(self, number, version, test_blocks = [], finaltx = False):
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
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            self.height += 1
            self.finaltx_vin = []
            for txout in tx_final.vout:
                self.finaltx_vin.append(CTxIn(COutPoint(tx_final.sha256, 0), CScript([]), 0xffffffff))
        return test_blocks

    def get_bip9_status(self, key):
        info = self.nodes[0].getblockchaininfo()
        if key in info['bip9_softforks']:
            return info['bip9_softforks'][key]
        else:
            raise KeyError(repr(info['bip9_softforks']))
            return info['bip9_softforks']['!' + key]

    def get_tests(self):
        bipName = 'finaltx'
        bitno = 1
        activated_version = 0x20000000 | (1 << bitno)

        assert_equal(self.get_bip9_status(bipName)['status'], 'defined')
        assert_equal(self.get_bip9_status(bipName)['since'], 0)

        # Generate some blocks to get the chain going and un-stick the mining RPCs
        self.nodes[0].generate(2)
        assert_equal(self.nodes[0].getblockcount(), 2)
        self.height = 3  # height of the next block to build
        self.tip = int("0x" + self.nodes[0].getbestblockhash(), 0)
        self.nodeaddress = self.nodes[0].getnewaddress()
        self.last_block_time = int(time.time())

        # bipName begins in DEFINED state
        assert_equal(self.get_bip9_status(bipName)['status'], 'defined')
        assert_equal(self.get_bip9_status(bipName)['since'], 0)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert(bipName not in tmpl['rules'])
        assert(bipName not in tmpl['vbavailable'])
        assert('finaltx' not in tmpl)
        assert_equal(tmpl['vbrequired'], 0)
        assert_equal(tmpl['version'] & activated_version, 0x20000000)

        # Test 1
        # Advance from DEFINED to STARTED
        test_blocks = self.generate_blocks(141, 4) # height = 143
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['status'], 'started')
        assert_equal(self.get_bip9_status(bipName)['since'], 144)
        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 0)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 0)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert(bipName not in tmpl['rules'])
        assert_equal(tmpl['vbavailable'][bipName], bitno)
        assert_equal(tmpl['vbrequired'], 0)
        assert('finaltx' not in tmpl)
        assert_equal(tmpl['version'] & activated_version, activated_version)

        # Save one of the anyone-can-spend coinbase outputs for later.
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].nValue, 5000000000)
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].scriptPubKey, CScript([OP_TRUE]))
        early_coin = COutPoint(test_blocks[-1][0].vtx[0].sha256, 0)

        # Test 2
        # check stats after max number of "signalling not" blocks such that LOCKED_IN still possible this period
        test_blocks = self.generate_blocks(36, 4, test_blocks) # 0x00000004 (signalling not)
        test_blocks = self.generate_blocks(10, activated_version) # 0x20000001 (signalling ready)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 46)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 10)
        assert_equal(self.get_bip9_status(bipName)['statistics']['possible'], True)

        # Test 3
        # check stats after one additional "signalling not" block --  LOCKED_IN no longer possible this period
        test_blocks = self.generate_blocks(1, 4, test_blocks) # 0x00000004 (signalling not)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 47)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 10)
        assert_equal(self.get_bip9_status(bipName)['statistics']['possible'], False)

        # Test 4
        # finish period with "ready" blocks, but soft fork will still fail to advance to LOCKED_IN
        test_blocks = self.generate_blocks(97, activated_version) # 0x20000001 (signalling ready)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 0)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 0)
        assert_equal(self.get_bip9_status(bipName)['statistics']['possible'], True)
        assert_equal(self.get_bip9_status(bipName)['status'], 'started')

        # Test 5
        # Fail to achieve LOCKED_IN 100 out of 144 signal bit 1
        # using a variety of bits to simulate multiple parallel softforks
        test_blocks = self.generate_blocks(50, activated_version) # 0x20000001 (signalling ready)
        test_blocks = self.generate_blocks(20, 4, test_blocks) # 0x00000004 (signalling not)
        test_blocks = self.generate_blocks(50, activated_version, test_blocks) # 0x20000101 (signalling ready)
        test_blocks = self.generate_blocks(24, 4, test_blocks) # 0x20010000 (signalling not)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['status'], 'started')
        assert_equal(self.get_bip9_status(bipName)['since'], 144)
        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 0)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 0)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert(bipName not in tmpl['rules'])
        assert_equal(tmpl['vbavailable'][bipName], bitno)
        assert_equal(tmpl['vbrequired'], 0)
        assert_equal(tmpl['version'] & activated_version, activated_version)

        # Test 6
        # 108 out of 144 signal bit 1 to achieve LOCKED_IN
        # using a variety of bits to simulate multiple parallel softforks
        test_blocks = self.generate_blocks(57, activated_version) # 0x20000001 (signalling ready)
        test_blocks = self.generate_blocks(26, 4, test_blocks) # 0x00000004 (signalling not)
        test_blocks = self.generate_blocks(50, activated_version, test_blocks) # 0x20000101 (signalling ready)
        test_blocks = self.generate_blocks(10, 4, test_blocks) # 0x20010000 (signalling not)
        yield TestInstance(test_blocks, sync_every_block=False)

        # check counting stats and "possible" flag before last block of this period achieves LOCKED_IN...
        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 143)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 107)
        assert_equal(self.get_bip9_status(bipName)['statistics']['possible'], True)
        assert_equal(self.get_bip9_status(bipName)['status'], 'started')

        # ...continue with Test 6 (now Test 7)
        test_blocks = self.generate_blocks(1, activated_version) # 0x20000001 (signalling ready)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['status'], 'locked_in')
        assert_equal(self.get_bip9_status(bipName)['since'], 576)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert(bipName not in tmpl['rules'])

        # Test 8
        # 143 more version 536870913 blocks (waiting period-1)
        test_blocks = self.generate_blocks(143, 4)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['status'], 'locked_in')
        assert_equal(self.get_bip9_status(bipName)['since'], 576)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert(bipName not in tmpl['rules'])
        assert(bipName in tmpl['vbavailable'])
        assert_equal(tmpl['vbrequired'], 0)
        assert_equal(tmpl['version'] & activated_version, activated_version)

        # Test 9
        # Generate a block without any spendable outputs, which should
        # be allowed under normal circumstances.
        test_blocks = self.generate_blocks(1, 4)
        for txout in test_blocks[-1][0].vtx[0].vout:
            txout.scriptPubKey = CScript([OP_FALSE])
        test_blocks[-1][0].vtx[0].rehash()
        test_blocks[-1][0].hashMerkleRoot = test_blocks[-1][0].calc_merkle_root()
        test_blocks[-1][0].rehash()
        test_blocks[-1][0].solve()
        yield TestInstance(test_blocks, sync_every_block=False)
        self.tip = test_blocks[-1][0].sha256

        assert_equal(self.get_bip9_status(bipName)['status'], 'active')
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert('finaltx' in tmpl['rules'])
        assert(bipName not in tmpl['vbavailable'])
        assert_equal(tmpl['vbrequired'], 0)
        assert(not (tmpl['version'] & (1 << bitno)))

        # Test 10
        # Attempt to do the same thing: generate a block with no
        # spendable outputs in the coinbase. This fails because the
        # next block needs at least one trivially spendable output to
        # start the block-final transaction chain.
        block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
        block.nVersion = 5
        for txout in block.vtx[0].vout:
            txout.scriptPubKey = CScript([OP_FALSE])
        block.vtx[0].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        yield TestInstance([[block, False]])

        # Test 11
        # Generate the first block with block-final-tx rules enforced, which
        # reuires the coinbase to have a trivially-spendable output.
        test_blocks = self.generate_blocks(1, 4)
        assert(any(out.scriptPubKey==CScript([OP_TRUE]) for out in test_blocks[-1][0].vtx[0].vout))
        for n,txout in enumerate(test_blocks[-1][0].vtx[0].vout):
            non_protected_output = COutPoint(test_blocks[-1][0].vtx[0].sha256, n)
            assert_equal(txout.nValue, 312500000)
        yield TestInstance(test_blocks, sync_every_block=False)

        # Test 12
        # Generate 98 blocks (maturity period - 2)
        test_blocks = self.generate_blocks(98, 4)
        yield TestInstance(test_blocks, sync_every_block=False)

        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert('finaltx' not in tmpl)

        # Test 13
        # Generate one more block to allow non_protected_output to mature, which
        # causes the block-final transaction to be required in the next block.
        test_blocks = self.generate_blocks(1, 4)
        yield TestInstance(test_blocks, sync_every_block=False)

        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert('finaltx' in tmpl)
        assert_equal(len(tmpl['finaltx']['prevout']), 1)
        assert_equal(tmpl['finaltx']['prevout'][0]['txid'], encode(ser_uint256(non_protected_output.hash)[::-1], 'hex_codec').decode('ascii'))
        assert_equal(tmpl['finaltx']['prevout'][0]['vout'], non_protected_output.n)
        assert_equal(tmpl['finaltx']['prevout'][0]['amount'], 312470199)

        # Extra pass-through value is not included in the coinbasevalue field.
        assert_equal(tmpl['coinbasevalue'], 5000000000 // 2**(self.height // 150))

        # The transactions field does not contain the block-final transaction.
        assert_equal(len(tmpl['transactions']), 0)

        # Test 14
        # Attempt to create a block without the block-final transaction, which
        # fails.
        block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
        block.nVersion = 4
        block.rehash()
        block.solve()
        yield TestInstance([[block, False]])

        # Test 15
        # Add the block-final transaction, and it passes.
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
        test_blocks.append([block, True])
        yield TestInstance(test_blocks, sync_every_block=False)
        prev_final_tx = block.vtx[-1]
        self.last_block_time += 1
        self.tip = block.sha256
        self.height += 1

        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit',bipName,'auxpow']})
        assert('finaltx' in tmpl)
        assert_equal(len(tmpl['finaltx']['prevout']), 1)
        assert_equal(tmpl['finaltx']['prevout'][0]['txid'], encode(ser_uint256(tx_final.sha256)[::-1], 'hex_codec').decode('ascii'))
        assert_equal(tmpl['finaltx']['prevout'][0]['vout'], 0)
        assert_equal(tmpl['finaltx']['prevout'][0]['amount'], 312469901)

        # Test 16
        # Create a block-final transaction with multiple outputs, which doesn't
        # work because the number of outputs is restricted to be no greater than
        # the number of inputs.
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
        yield TestInstance([[block, False]])

        # Test 17
        # Try increasing the number of inputs by using an old one doesn't work,
        # because the block-final transaction must source its user inputs from
        # the same block.
        utxo = self.nodes[0].gettxout(encode(ser_uint256(early_coin.hash)[::-1], 'hex_codec').decode('ascii'), early_coin.n)
        assert('amount' in utxo)
        utxo_amount = int(100000000 * utxo['amount'])
        block.vtx[-1].vin.append(CTxIn(early_coin, CScript([]), 0xffffffff))
        block.vtx[-1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        yield TestInstance([[block, False]])

        # Test 18
        # But spend it via a user transaction instead, and it can be captured
        # and sent to the coinbase as fee.

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
        test_blocks.append([block, True])
        yield TestInstance(test_blocks, sync_every_block=False)
        prev_final_tx = block.vtx[-1]
        self.last_block_time += 1
        self.tip = block.sha256
        self.height += 1

        # Test 19
        # Spending only one of the prior outputs is insufficient.  ALL prior
        # block-final outputs must be spent.
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
        yield TestInstance([[block, False]])

        # Test 20
        # But spend all the prior outputs and it goes through.
        block.vtx[-1].vin.append(CTxIn(COutPoint(prev_final_tx.sha256, 1), CScript([]), 0xffffffff))
        block.vtx[-1].vout[0].nValue *= 2
        block.vtx[-1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        test_blocks.append([block, True])
        yield TestInstance(test_blocks, sync_every_block=False)
        prev_final_tx = block.vtx[-1]
        self.last_block_time += 1
        self.tip = block.sha256
        self.height += 1

        # Test 20 (con't)
        # (Don't increment the test number because we're not using TestInstance,
        # but this is something unrelated.)
        #
        # Now that the rules have activated, transactions spending the previous
        # block-final transaction's outputs are rejected from the mempool.
        #
        # First we do this with a non-block-final input to demonstrate the test
        # would otherwise work.
        height = self.nodes[0].getblockcount() - 99
        while True:
            blk = self.nodes[0].getblock(self.nodes[0].getblockhash(height))
            txid = blk['tx'][0]
            utxo = self.nodes[0].gettxout(txid, 0)
            if utxo is not None and utxo['scriptPubKey']['hex'] == "51":
                break
            height -= 1

        spend_tx = CTransaction()
        spend_tx.nVersion = 2
        spend_tx.vin.append(CTxIn(COutPoint(uint256_from_str(unhexlify(txid)[::-1]), 0), CScript([]), 0xffffffff))
        spend_tx.vout.append(CTxOut(int(utxo['amount']*100000000) - 10000, CScript([b'a'*100]))) # Make transaction large enough to avoid tx-size-small standardness check
        spend_tx.nLockTime = 0
        spend_tx.lock_height = utxo['refheight']
        spend_tx.rehash()
        self.nodes[0].sendrawtransaction(ToHex(spend_tx))
        mempool = self.nodes[0].getrawmempool()
        assert(spend_tx.hash in mempool)

        # Now we do the same exact thing with the last block-final transaction's
        # outputs. It should not enter the mempool.
        spend_tx = CTransaction()
        spend_tx.nVersion = 2
        spend_tx.vin.append(CTxIn(COutPoint(prev_final_tx.sha256, 0), CScript([]), 0xffffffff))
        spend_tx.vout.append(CTxOut(int(utxo['amount']*100000000), CScript([b'a'*100]))) # Make transaction large enough to avoid tx-size-small standardness check
        spend_tx.nLockTime = 0
        spend_tx.lock_height = utxo['refheight']
        spend_tx.rehash()
        try:
            self.nodes[0].sendrawtransaction(ToHex(spend_tx))
        except JSONRPCException as e:
            assert("spend-block-final-txn" in e.error['message'])
        else:
            assert(False)
        mempool = self.nodes[0].getrawmempool()
        assert(spend_tx.hash not in mempool)

        # Test 17
        # Invalidate the tip, then malleate and re-solve the same block. This is
        # a fast way of testing test that the block-final txid is restored on a
        # reorg.
        height = self.nodes[0].getblockcount()
        self.nodes[0].invalidateblock(test_blocks[-1][0].hash)
        assert_equal(self.nodes[0].getblockcount(), height-1)
        test_blocks[-1][0].nVersion ^= 2
        test_blocks[-1][0].rehash()
        test_blocks[-1][0].solve()
        yield TestInstance(test_blocks, sync_every_block=False)
        assert_equal(self.nodes[0].getblockcount(), height)
        assert_equal(self.nodes[0].getblockhash(height), test_blocks[-1][0].hash)
        self.tip = test_blocks[-1][0].sha256
        self.finaltx_vin = [CTxIn(COutPoint(test_blocks[-1][0].vtx[-1].sha256, 0), CScript([]), 0xffffffff)]

        # Test 18-21
        # Mine two blocks with trivially-spendable coinbase outputs, then test
        # that the one that is exactly 100 blocks old is allowed to be spent in
        # a block-final transaction, but the older one cannot.
        test_blocks = self.generate_blocks(1, 4, finaltx=True)
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].scriptPubKey, CScript([OP_TRUE]))
        txin1 = CTxIn(COutPoint(test_blocks[-1][0].vtx[0].sha256, 0), CScript([]), 0xffffffff)

        test_blocks = self.generate_blocks(1, 4, test_blocks, finaltx=True)
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].scriptPubKey, CScript([OP_TRUE]))
        txin2 = CTxIn(COutPoint(test_blocks[-1][0].vtx[0].sha256, 0), CScript([]), 0xffffffff)

        test_blocks = self.generate_blocks(1, 4, test_blocks, finaltx=True)
        assert_equal(test_blocks[-1][0].vtx[0].vout[0].scriptPubKey, CScript([OP_TRUE]))
        txin3 = CTxIn(COutPoint(test_blocks[-1][0].vtx[0].sha256, 0), CScript([]), 0xffffffff)

        test_blocks = self.generate_blocks(98, 4, test_blocks, finaltx=True)
        yield TestInstance(test_blocks, sync_every_block=False)

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
        test_blocks.append([block, False])
        yield TestInstance(test_blocks, sync_every_block=False)

        # txin3 is too young -- it hasn't matured
        block.vtx[-1].vin.pop()
        block.vtx[-1].vin.append(txin3)
        block.vtx[-1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        test_blocks.pop()
        test_blocks.append([block, False])
        yield TestInstance(test_blocks, sync_every_block=False)

        # txin2 is just right
        block.vtx[-1].vin.pop()
        block.vtx[-1].vin.append(txin2)
        block.vtx[-1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        test_blocks.pop()
        test_blocks.append([block, True])
        yield TestInstance(test_blocks, sync_every_block=False)

        self.last_block_time += 1
        self.tip = test_blocks[-1][0].sha256
        self.height += 1
        self.finaltx_vin = [CTxIn(COutPoint(test_blocks[-1][0].vtx[-1].sha256, 0), CScript([]), 0xffffffff)]

        # Restart all
        self.test.clear_all_connections()
        self.stop_nodes()
        self.nodes = []
        shutil.rmtree(self.options.tmpdir + "/node0")
        self.setup_chain()
        self.setup_network()
        self.test.add_all_connections(self.nodes)
        NetworkThread().start()
        self.test.p2p_connections[0].wait_for_verack()

        return

if __name__ == '__main__':
    BlockFinalTxTest().main()
