// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "uint256.h"
#include "util.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(miner_tests)

static
struct {
    unsigned char extranonce;
    unsigned int nonce;
} blockinfo[] = {
    {2, 0x64287fa7}, {1, 0x879845ca}, {1, 0x481a20d6}, {0, 0x2fafb9e2},
    {0, 0x398a537c}, {1, 0x869e318a}, {0, 0xde2e37e9}, {0, 0x19e8012e},
    {0, 0x05c18aa3}, {1, 0x913d7ea2}, {0, 0x6d4019e4}, {1, 0x19dbe5fe},
    {0, 0x72e8f50b}, {3, 0x83c03e72}, {0, 0x3cb3dc3d}, {0, 0x2f3ec481},
    {0, 0x257c60cc}, {0, 0x06296904}, {0, 0xb390c08a}, {0, 0xea447344},
    {0, 0xf95fa035}, {0, 0x123d792e}, {0, 0x338dbd37}, {0, 0x6a945b6a},
    {1, 0xcab85a93}, {0, 0x282710d1}, {1, 0xc9c31c08}, {2, 0x5f2a7549},
    {0, 0xd4ec83b1}, {0, 0x59db2b73}, {1, 0xe594668c}, {0, 0xc60e3d7c},
    {0, 0x0280afa1}, {2, 0x6ee0b70e}, {0, 0xaa84c777}, {0, 0x41be9899},
    {1, 0x67d4e259}, {0, 0x710a10c8}, {0, 0x12f6404e}, {0, 0x3f56283b},
    {1, 0x960e8916}, {3, 0x458b90e2}, {4, 0xe0ea2e58}, {0, 0xcf85f74b},
    {0, 0x21201a02}, {0, 0xe33c6a6d}, {1, 0x0f191ba3}, {0, 0x26ddaad4},
    {0, 0x68c4d548}, {0, 0x44798d6f}, {0, 0x6b431584}, {0, 0xc32db943},
    {1, 0xac271fb5}, {1, 0xdfeceaad}, {0, 0x34b07f4a}, {0, 0x4a625753},
    {0, 0xd9f716fd}, {0, 0x7f1437c0}, {0, 0xf8e32347}, {1, 0x6986eb1c},
    {0, 0xddf0cea9}, {0, 0xa6e90985}, {0, 0xbe5a58f4}, {1, 0x2219d81e},
    {1, 0x407854c3}, {1, 0xc95baa1d}, {0, 0x67050137}, {0, 0x6252b8ed},
    {1, 0xebb0eaae}, {1, 0x2e1c502b}, {0, 0x83a71b98}, {0, 0xce6fdd67},
    {0, 0x477d1ad5}, {1, 0xee8c9329}, {0, 0x86445f07}, {5, 0xbc6379c9},
    {0, 0xb3d38698}, {0, 0x33e95c13}, {0, 0x0775ccbf}, {1, 0x2091eea9},
    {1, 0x5d8011c7}, {1, 0x664d220c}, {2, 0xa94c3bb0}, {2, 0x7b09128d},
    {1, 0x57e45428}, {0, 0xb06872ad}, {0, 0x35dc4550}, {0, 0x8994a8e7},
    {1, 0xc9438b5f}, {0, 0x1c8230b7}, {0, 0x2fc1c1fe}, {0, 0x42c9ddd0},
    {1, 0x40b4eeb0}, {0, 0x0b2832a2}, {0, 0x60d8148f}, {1, 0xadb4835b},
    {1, 0x8e6a1a55}, {0, 0x8f341684}, {1, 0x1f29a805}, {0, 0xa2098864},
    {0, 0x1b971472}, {1, 0x44a6ec78}, {2, 0x08320965}, {0, 0xf6154f9e},
    {0, 0xc3db8219}, {0, 0x366b2460}, {2, 0xca10a1eb}, {0, 0x594e0a1d},
    {0, 0x526a8b7d}, {0, 0xf5fda6eb}
};

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;
    CMutableTransaction tx,tx2;
    CScript script;
    uint256 hash;

    LOCK(cs_main);
    Checkpoints::fEnabled = false;

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));

    // We can't make transactions until we have inputs
    // Therefore, load 100 blocks :)
    std::vector<CTransaction*>txFirst;
    for (unsigned int i = 0; i < sizeof(blockinfo)/sizeof(*blockinfo); ++i)
    {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 1;
        pblock->nTime = chainActive.Tip()->GetMedianTimePast()+1;
        CMutableTransaction txCoinbase(pblock->vtx[0]);
        txCoinbase.nVersion = 2;
        txCoinbase.vin[0].scriptSig = CScript();
        txCoinbase.vin[0].scriptSig << static_cast<int64_t>(chainActive.Height() + 1);
        txCoinbase.vin[0].scriptSig << CScriptNum(blockinfo[i].extranonce);
        txCoinbase.vout[0].nValue = 50 * COIN;
        txCoinbase.vout[0].scriptPubKey = CScript();
        txCoinbase.lock_height = chainActive.Height() + 1;
        pblock->vtx[0] = CTransaction(txCoinbase);
        if (txFirst.size() < 2)
            txFirst.push_back(new CTransaction(pblock->vtx[0]));
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();
        pblock->nNonce = blockinfo[i].nonce;
        CValidationState state;
        BOOST_CHECK(ProcessNewBlock(state, NULL, pblock));
        BOOST_CHECK(state.IsValid());
        pblock->hashPrevBlock = pblock->GetHash();
    }
    delete pblocktemplate;

    // Just to make sure we can still make simple blocks
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;

    tx.lock_height = txFirst[1]->lock_height;
    tx2.lock_height = tx.lock_height;

    // block sigops > limit: 1000 CHECKMULTISIG + 1
    tx.vin.resize(1);
    // NOTE: OP_NOP is used to force 20 SigOps for the CHECKMULTISIG
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_0 << OP_0 << OP_NOP << OP_CHECKMULTISIG << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 5000000000LL;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= 1000000;
        hash = tx.GetHash();
        mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // block size > limit
    tx.vin[0].scriptSig = CScript();
    // 18 * (520char + DROP) + OP_1 = 9433 bytes
    std::vector<unsigned char> vchData(520);
    for (unsigned int i = 0; i < 18; ++i)
        tx.vin[0].scriptSig << vchData << OP_DROP;
    tx.vin[0].scriptSig << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = 5000000000LL;
    for (unsigned int i = 0; i < 128; ++i)
    {
        tx.vout[0].nValue -= 10000000;
        hash = tx.GetHash();
        mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // orphan in mempool
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // child with higher priority than parent
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = 4900000000LL;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    tx.vin[0].prevout.hash = hash;
    tx.vin.resize(2);
    tx.vin[1].scriptSig = CScript() << OP_1;
    tx.vin[1].prevout.hash = txFirst[0]->GetHash();
    tx.vin[1].prevout.n = 0;
    tx.vout[0].nValue = 5900000000LL;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // coinbase in mempool
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
    tx.vout[0].nValue = 0;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // invalid (pre-p2sh) txn in mempool
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = 4900000000LL;
    script = CScript() << OP_0;
    tx.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(script));
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    tx.vin[0].prevout.hash = hash;
    tx.vin[0].scriptSig = CScript() << (std::vector<unsigned char>)script;
    tx.vout[0].nValue -= 1000000;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // double spend txn pair in mempool
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = 4900000000LL;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    tx.vout[0].scriptPubKey = CScript() << OP_2;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // subsidy changing
    int nHeight = chainActive.Height();
    chainActive.Tip()->nHeight = 209999;
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    chainActive.Tip()->nHeight = 210000;
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    chainActive.Tip()->nHeight = nHeight;

    // non-final txs in mempool
    SetMockTime(chainActive.Tip()->GetMedianTimePast()+1);

    // height locked
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].nSequence = 0;
    tx.vout[0].nValue = 4900000000LL;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    tx.nLockTime = chainActive.Tip()->nHeight+1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(!IsFinalTx(tx, chainActive.Tip()->nHeight + 1));

    // time locked
    tx2.vin.resize(1);
    tx2.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx2.vin[0].prevout.n = 0;
    tx2.vin[0].scriptSig = CScript() << OP_1;
    tx2.vin[0].nSequence = 0;
    tx2.vout.resize(1);
    tx2.vout[0].nValue = 4900000000LL;
    tx2.vout[0].scriptPubKey = CScript() << OP_1;
    tx2.nLockTime = chainActive.Tip()->GetMedianTimePast()+1;
    hash = tx2.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx2, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(!IsFinalTx(tx2));

    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));

    // Neither tx should have make it into the template.
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 1);
    delete pblocktemplate;

    // However if we advance height and time by one, both will.
    chainActive.Tip()->nHeight++;
    SetMockTime(chainActive.Tip()->GetMedianTimePast()+2);

    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 1));
    BOOST_CHECK(IsFinalTx(tx2));

    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    delete pblocktemplate;

    chainActive.Tip()->nHeight--;
    SetMockTime(0);
    mempool.clear();

    // Test non-monotonic lock_height by creating two dependent
    // transactions where the second transaction has a lower
    // lock_height than the first. This shouldn't pass validation and
    // shouldn't make it into a block template.
    tx = CTransaction();
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].nSequence = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 2450000000LL;
    tx.vout[0].scriptPubKey = CScript();
    tx.lock_height = chainActive.Tip()->nHeight + 1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));

    tx2 = CTransaction();
    tx2.vin.resize(1);
    tx2.vin[0].prevout.hash = hash;
    tx2.vin[0].prevout.n = 0;
    tx2.vin[0].scriptSig = CScript() << OP_1;
    tx2.vin[0].nSequence = 0;
    tx2.vout.resize(1);
    tx2.vout[0].nValue = 1225000000LL;
    tx2.vout[0].scriptPubKey = CScript();
    tx2.lock_height = chainActive.Tip()->nHeight;
    hash = tx2.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx2, 11, GetTime(), 111.0, 11));

    // Only the first transaction makes it into the template
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 1));
    BOOST_CHECK(IsFinalTx(tx2, chainActive.Tip()->nHeight + 1));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 2);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1].GetHash() == tx.GetHash());

    // Now we try connecting the block to engage consensus code checks
    // on monotonic lock_heights.
    {
        // The block with one transaction would be valid, if mined
        CValidationState state;
        BOOST_CHECK_MESSAGE(TestBlockValidity(state, pblocktemplate->block, chainActive.Tip(), false, false), state.GetRejectReason());
    }
    {
        // But force inclusion of the second transaction, and it fails
        pblocktemplate->block.vtx.resize(3);
        pblocktemplate->block.vtx[2] = tx2;
        CValidationState state;
        BOOST_CHECK(!TestBlockValidity(state, pblocktemplate->block, chainActive.Tip(), false, false));
        BOOST_CHECK(state.GetRejectCode() == REJECT_INVALID);
        BOOST_CHECK_MESSAGE(state.GetRejectReason() == "bad-txns-non-monotonic-lock-height", state.GetRejectReason());
    }

    delete pblocktemplate;
    mempool.clear();

    // Change the lock_height to be the same and it works
    ++tx2.lock_height;
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 1));
    BOOST_CHECK(IsFinalTx(tx2, chainActive.Tip()->nHeight + 1));
    mempool.addUnchecked(tx.GetHash(), CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    mempool.addUnchecked(tx2.GetHash(), CTxMemPoolEntry(tx2, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1].GetHash() == tx.GetHash());
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 3 && pblocktemplate->block.vtx[2].GetHash() == tx2.GetHash());

    {
        CValidationState state;
        BOOST_CHECK_MESSAGE(TestBlockValidity(state, pblocktemplate->block, chainActive.Tip(), false, false), state.GetRejectReason());
    }

    delete pblocktemplate;
    mempool.clear();

    // However a strictly increasing block height would run afoul of
    // the rule that lock_heights not exceed the current block height
    ++tx2.lock_height;
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 1));
    BOOST_CHECK(!IsFinalTx(tx2, chainActive.Tip()->nHeight + 1));
    mempool.addUnchecked(tx.GetHash(), CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    mempool.addUnchecked(tx2.GetHash(), CTxMemPoolEntry(tx2, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 2);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1].GetHash() == tx.GetHash());

    {
        // The block with one transaction would be valid, if mined
        CValidationState state;
        BOOST_CHECK_MESSAGE(TestBlockValidity(state, pblocktemplate->block, chainActive.Tip(), false, false), state.GetRejectReason());
    }
    {
        // But force inclusion of the second transaction, and it fails
        pblocktemplate->block.vtx.resize(3);
        pblocktemplate->block.vtx[2] = tx2;
        CValidationState state;
        BOOST_CHECK(!TestBlockValidity(state, pblocktemplate->block, chainActive.Tip(), false, false));
        BOOST_CHECK(state.GetRejectCode() == REJECT_INVALID);
        BOOST_CHECK_MESSAGE(state.GetRejectReason() == "bad-txns-nonfinal", state.GetRejectReason());
    }

    delete pblocktemplate;
    mempool.clear();

    // Force-increment the current block height and the second
    // transaction becomes valid again
    chainActive.Tip()->nHeight++;
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 1));
    BOOST_CHECK(IsFinalTx(tx2, chainActive.Tip()->nHeight + 1));
    mempool.addUnchecked(tx.GetHash(), CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    mempool.addUnchecked(tx2.GetHash(), CTxMemPoolEntry(tx2, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1].GetHash() == tx.GetHash());
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 3 && pblocktemplate->block.vtx[2].GetHash() == tx2.GetHash());

    {
        CValidationState state;
        BOOST_CHECK_MESSAGE(TestBlockValidity(state, pblocktemplate->block, chainActive.Tip(), false, false), state.GetRejectReason());
    }

    delete pblocktemplate;
    mempool.clear();

    BOOST_FOREACH(CTransaction *tx, txFirst)
        delete tx;

    Checkpoints::fEnabled = true;
}

BOOST_AUTO_TEST_SUITE_END()
