// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
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

#include "chainparams.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "script/standard.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"

#include "test/test_freicoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(miner_tests, TestingSetup)

static
struct {
    unsigned char extranonce;
    unsigned int nonce;
} blockinfo[] = {
    {1, 0x52d00042}, {0, 0x8e9502af}, {0, 0x0c9086ed}, {1, 0x4dafe8b7},
    {0, 0x2284fd8e}, {1, 0xbe19c94e}, {0, 0x944fb443}, {0, 0x918164c2},
    {0, 0x5ca9005e}, {2, 0xdfd36b8c}, {1, 0xbea0958d}, {1, 0x63157720},
    {0, 0xc7c138b1}, {1, 0x47abc96b}, {0, 0x1a41ae50}, {0, 0x33f81ee2},
    {0, 0x93d37ee6}, {0, 0xd41309ef}, {1, 0xe030f49b}, {0, 0xe92ac447},
    {2, 0x0a89381e}, {0, 0x23173160}, {0, 0x2dc1d884}, {0, 0xaba6e9b6},
    {0, 0xbaa9b8fb}, {0, 0x31d86401}, {0, 0x7ab38bc3}, {0, 0xb6a5921c},
    {1, 0x46901dbf}, {1, 0x55c74e05}, {0, 0x5236e3b6}, {0, 0xa09faea4},
    {1, 0x239feb9a}, {0, 0x10021b9a}, {1, 0x1edc56aa}, {3, 0x9d65337b},
    {0, 0x790bf7c1}, {0, 0x2512d056}, {5, 0xcfd1ad26}, {0, 0xfc101a5a},
    {0, 0x7094b9df}, {0, 0x76cb9875}, {0, 0xc2405a3d}, {0, 0x7b63fe34},
    {0, 0x0db81fd0}, {0, 0xceceadd6}, {0, 0x95f2ea4b}, {0, 0x4c77115a},
    {1, 0xa6b53f07}, {1, 0xb526245d}, {0, 0x2d4b00c8}, {1, 0xc35c0090},
    {1, 0xaada2eda}, {0, 0x3fd18fab}, {0, 0x7747abf2}, {2, 0xb0b92b17},
    {0, 0x67227e86}, {0, 0xf9fd9455}, {0, 0x69251c19}, {0, 0x633da320},
    {0, 0x8acefe47}, {0, 0xeb82586d}, {1, 0x121792ef}, {4, 0x3ef76f0e},
    {1, 0xb4f30ec8}, {0, 0x4245e33b}, {1, 0xb9bd34c0}, {0, 0x293a1297},
    {0, 0x3843f35f}, {1, 0x5f6ceef1}, {1, 0xcb3a5a97}, {3, 0xa0c5c696},
    {0, 0xce5b8825}, {0, 0xe5bad1e6}, {2, 0x2e775dcb}, {1, 0xdd7ba849},
    {0, 0x70ddc54b}, {1, 0x4d7291de}, {2, 0xdac7f375}, {0, 0xb48a1cbc},
    {0, 0x28e05f52}, {2, 0x833fba3d}, {1, 0x09260f05}, {2, 0x46c7653e},
    {2, 0x1b02eee1}, {0, 0x4c6dab99}, {0, 0x758ec784}, {1, 0x6963c9fa},
    {0, 0x8b92a0b4}, {0, 0x6c82722c}, {0, 0x09c12660}, {0, 0xd02bdc4c},
    {1, 0xb89cdf2d}, {0, 0x107ce7da}, {0, 0x525613a2}, {0, 0xbc717ed5},
    {0, 0x477812dc}, {0, 0x8a6d8f17}, {0, 0x9df124b3}, {0, 0x461e1332},
    {0, 0x0dde35c2}, {0, 0x76683cec}, {0, 0x92a5017b}, {0, 0x681fb885},
    {2, 0x617826d0}, {0, 0x0029a8e6}, {0, 0x1d3108a1}, {4, 0x33dc89e2},
    {0, 0x520883b9}, {0, 0xfb702683}
};

CBlockIndex CreateBlockIndex(int nHeight)
{
    CBlockIndex index;
    index.nHeight = nHeight;
    index.pprev = chainActive.Tip();
    return index;
}

bool TestSequenceLocks(const CTransaction &tx, int flags)
{
    LOCK(mempool.cs);
    return CheckSequenceLocks(tx, flags);
}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    const CChainParams& chainparams = Params(CBaseChainParams::MAIN);
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;
    CMutableTransaction tx,tx2;
    CScript script;
    uint256 hash;
    TestMemPoolEntryHelper entry;
    entry.nFee = 11;
    entry.dPriority = 111.0;
    entry.nHeight = 11;

    LOCK(cs_main);
    fCheckpointsEnabled = false;

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));

    // We can't make transactions until we have inputs
    // Therefore, load 100 blocks :)
    int baseheight = 0;
    std::vector<CTransaction*>txFirst;
    for (unsigned int i = 0; i < sizeof(blockinfo)/sizeof(*blockinfo); ++i)
    {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 2;
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
        if (txFirst.size() == 0)
            baseheight = chainActive.Height();
        if (txFirst.size() < 4)
            txFirst.push_back(new CTransaction(pblock->vtx[0]));
        pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
        pblock->nNonce = blockinfo[i].nonce;
        CValidationState state;
        BOOST_CHECK(ProcessNewBlock(state, chainparams, NULL, pblock, true, NULL));
        BOOST_CHECK(state.IsValid());
        pblock->hashPrevBlock = pblock->GetHash();
    }
    delete pblocktemplate;

    // Just to make sure we can still make simple blocks
    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    delete pblocktemplate;

    tx.lock_height = txFirst.back()->lock_height;
    tx2.lock_height = tx.lock_height;

    // block sigops > limit: 1000 CHECKMULTISIG + 1
    tx.vin.resize(1);
    // NOTE: OP_NOP is used to force 20 SigOps for the CHECKMULTISIG
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_0 << OP_0 << OP_NOP << OP_CHECKMULTISIG << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 4900000000LL;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= 1000000;
        hash = tx.GetHash();
        bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
        // If we don't set the # of sig ops in the CTxMemPoolEntry, template creation fails
        mempool.addUnchecked(hash, entry.Fee(1000000).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK_THROW(CreateNewBlock(chainparams, scriptPubKey), std::runtime_error);
    mempool.clear();

    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = 4900000000LL;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= 1000000;
        hash = tx.GetHash();
        bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
        // If we do set the # of sig ops in the CTxMemPoolEntry, template creation passes
        mempool.addUnchecked(hash, entry.Fee(1000000).Time(GetTime()).SpendsCoinbase(spendsCoinbase).SigOps(20).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
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
    tx.vout[0].nValue = 4900000000LL;
    for (unsigned int i = 0; i < 128; ++i)
    {
        tx.vout[0].nValue -= 10000000;
        hash = tx.GetHash();
        bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
        mempool.addUnchecked(hash, entry.Fee(1000000).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // orphan in mempool, template creation fails
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(1000000).Time(GetTime()).FromTx(tx));
    BOOST_CHECK_THROW(CreateNewBlock(chainparams, scriptPubKey), std::runtime_error);
    mempool.clear();

    // child with higher priority than parent
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = 4900000000LL;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(100000000LL).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vin[0].prevout.hash = hash;
    tx.vin.resize(2);
    tx.vin[1].scriptSig = CScript() << OP_1;
    tx.vin[1].prevout.hash = txFirst[0]->GetHash();
    tx.vin[1].prevout.n = 0;
    tx.vout[0].nValue = 5900000000LL;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(400000000LL).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    delete pblocktemplate;
    mempool.clear();

    // coinbase in mempool, template creation fails
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
    tx.vout[0].nValue = 0;
    hash = tx.GetHash();
    // give it a fee so it'll get mined
    mempool.addUnchecked(hash, entry.Fee(100000).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    BOOST_CHECK_THROW(CreateNewBlock(chainparams, scriptPubKey), std::runtime_error);
    mempool.clear();

    // invalid (pre-p2sh) txn in mempool, template creation fails
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = 4900000000LL;
    script = CScript() << OP_0;
    tx.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(script));
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(10000000L).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vin[0].prevout.hash = hash;
    tx.vin[0].scriptSig = CScript() << std::vector<unsigned char>(script.begin(), script.end());
    tx.vout[0].nValue -= 1000000;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(1000000).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    BOOST_CHECK_THROW(CreateNewBlock(chainparams, scriptPubKey), std::runtime_error);
    mempool.clear();

    // double spend txn pair in mempool, template creation fails
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = 4900000000LL;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(100000000L).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vout[0].scriptPubKey = CScript() << OP_2;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(100000000L).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK_THROW(CreateNewBlock(chainparams, scriptPubKey), std::runtime_error);
    mempool.clear();

    // subsidy changing
    int nHeight = chainActive.Height();
    // Create an actual 209999-long block chain (without valid blocks).
    while (chainActive.Tip()->nHeight < 209999) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(GetRandHash());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        chainActive.SetTip(next);
    }
    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    delete pblocktemplate;
    // Extend to a 210000-long block chain.
    while (chainActive.Tip()->nHeight < 210000) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(GetRandHash());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        chainActive.SetTip(next);
    }
    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    delete pblocktemplate;
    // Delete the dummy blocks again.
    while (chainActive.Tip()->nHeight > nHeight) {
        CBlockIndex* del = chainActive.Tip();
        chainActive.SetTip(del->pprev);
        pcoinsTip->SetBestBlock(del->pprev->GetBlockHash());
        delete del->phashBlock;
        delete del;
    }

    // non-final txs in mempool
    SetMockTime(chainActive.Tip()->GetMedianTimePast()+1);
    int flags = LOCKTIME_VERIFY_SEQUENCE|LOCKTIME_MEDIAN_TIME_PAST;
    // height map
    std::vector<int> prevheights;

    // relative height locked
    tx.nVersion = 2;
    tx.vin.resize(1);
    prevheights.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash(); // only 1 transaction
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].nSequence = chainActive.Tip()->nHeight + 1; // txFirst[0] is the 2nd block
    prevheights[0] = baseheight + 1;
    tx.vout.resize(1);
    tx.vout[0].nValue = 4900000000LL;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    tx.nLockTime = 0;
    tx.lock_height = txFirst[0]->lock_height;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(100000000L).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    BOOST_CHECK(SequenceLocks(tx, flags, &prevheights, CreateBlockIndex(chainActive.Tip()->nHeight + 2))); // Sequence locks pass on 2nd block

    // relative time locked
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.lock_height = txFirst[1]->lock_height;
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | (((chainActive.Tip()->GetMedianTimePast()+1-chainActive[1]->GetMedianTimePast()) >> CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) + 1); // txFirst[1] is the 3rd block
    prevheights[0] = baseheight + 2;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail

    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    BOOST_CHECK(SequenceLocks(tx, flags, &prevheights, CreateBlockIndex(chainActive.Tip()->nHeight + 1))); // Sequence locks pass 512 seconds later
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime -= 512; //undo tricked MTP

    // absolute height locked
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.lock_height = txFirst[2]->lock_height;
    tx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL - 1;
    prevheights[0] = baseheight + 3;
    tx.nLockTime = chainActive.Tip()->nHeight + 1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTx(tx, flags)); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 2, chainActive.Tip()->GetMedianTimePast())); // Locktime passes on 2nd block

    // absolute time locked
    tx.vin[0].prevout.hash = txFirst[3]->GetHash();
    tx.lock_height = txFirst[3]->lock_height;
    tx.nLockTime = chainActive.Tip()->GetMedianTimePast();
    prevheights.resize(1);
    prevheights[0] = baseheight + 4;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTx(tx, flags)); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 2, chainActive.Tip()->GetMedianTimePast() + 1)); // Locktime passes 1 second later

    // mempool-dependent transactions (not added)
    tx.vin[0].prevout.hash = hash;
    prevheights[0] = chainActive.Tip()->nHeight + 1;
    tx.nLockTime = 0;
    tx.vin[0].nSequence = 0;
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    tx.vin[0].nSequence = 1;
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG;
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | 1;
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail

    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));

    // None of the of the absolute height/time locked tx should have made
    // it into the template because we still check IsFinalTx in CreateNewBlock,
    // but relative locked txs will if inconsistently added to mempool.
    // For now these will still generate a valid template until BIP68 soft fork
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    delete pblocktemplate;
    // However if we advance height by 1 and time by 512, all of them should be mined
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    chainActive.Tip()->nHeight++;
    SetMockTime(chainActive.Tip()->GetMedianTimePast() + 1);

    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 5);
    delete pblocktemplate;

    chainActive.Tip()->nHeight--;
    SetMockTime(0);
    mempool.clear();

    // To get around standardness rules we use an OP_TRUE script
    // behind a P2SH construction, and turn off fRequireStandard so
    // P2SH redeem scripts aren't checked.
    CScript p2shTrue = CScript() << OP_TRUE;
    bool old_fRequireStandard = fRequireStandard;
    fRequireStandard = false;

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
    tx.vout[0].nValue = 2500000000LL;
    tx.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(p2shTrue));
    tx.lock_height = chainActive.Tip()->nHeight + 1;
    hash = tx.GetHash();

    tx2 = CTransaction();
    tx2.vin.resize(1);
    tx2.vin[0].prevout.hash = hash;
    tx2.vin[0].prevout.n = 0;
    tx2.vin[0].scriptSig = CScript() << std::vector<unsigned char>(p2shTrue.begin(), p2shTrue.end());
    tx2.vin[0].nSequence = 0;
    tx2.vout.resize(1);
    tx2.vout[0].nValue = 1250000000LL;
    tx2.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(p2shTrue));
    tx2.lock_height = chainActive.Tip()->nHeight;
    hash = tx2.GetHash();

    // Both transactions are final, which doesn't consider context
    BOOST_CHECK(CheckFinalTx(tx));
    BOOST_CHECK(CheckFinalTx(tx2));

    // But only the first transaction makes it into the mempool
    bool res = false;
    CValidationState state;
    BOOST_CHECK(res = AcceptToMemoryPool(mempool, state, tx, false, NULL, true, false));
    BOOST_CHECK_MESSAGE(res, state.GetRejectReason());

    state = CValidationState();
    BOOST_CHECK(!AcceptToMemoryPool(mempool, state, tx2, false, NULL, true, false));

    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 2);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1].GetHash() == tx.GetHash());

    // Now we try connecting the block to engage consensus code checks
    // on monotonic lock_heights.

    // The block with one transaction would be valid, if mined
    state = CValidationState();
    BOOST_CHECK(res = TestBlockValidity(state, chainparams, pblocktemplate->block, chainActive.Tip(), false, false));
    BOOST_CHECK_MESSAGE(res, state.GetRejectReason());

    // But force inclusion of the second transaction, and it fails
    state = CValidationState();
    pblocktemplate->block.vtx.resize(3);
    pblocktemplate->block.vtx[2] = tx2;
    BOOST_CHECK(!TestBlockValidity(state, chainparams, pblocktemplate->block, chainActive.Tip(), false, false));
    BOOST_CHECK(state.GetRejectCode() == REJECT_INVALID);
    BOOST_CHECK_MESSAGE(state.GetRejectReason() == "bad-txns-non-monotonic-lock-height", state.GetRejectReason());

    delete pblocktemplate;
    mempool.clear();

    // Change the lock_height to be the same and it works
    ++tx2.lock_height;
    BOOST_CHECK(CheckFinalTx(tx));
    BOOST_CHECK(CheckFinalTx(tx2));

    state = CValidationState();
    BOOST_CHECK(res = AcceptToMemoryPool(mempool, state, tx, false, NULL, true, false));
    BOOST_CHECK_MESSAGE(res, state.GetRejectReason());

    state = CValidationState();
    BOOST_CHECK(res = AcceptToMemoryPool(mempool, state, tx2, false, NULL, true, false));
    BOOST_CHECK_MESSAGE(res, state.GetRejectReason());

    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1].GetHash() == tx.GetHash());
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 3 && pblocktemplate->block.vtx[2].GetHash() == tx2.GetHash());

    state = CValidationState();
    BOOST_CHECK_MESSAGE(TestBlockValidity(state, chainparams, pblocktemplate->block, chainActive.Tip(), false, false), state.GetRejectReason());

    delete pblocktemplate;
    mempool.clear();

    // However a strictly increasing block height would run afoul of
    // the rule that lock_heights not exceed the current block height
    ++tx2.lock_height;
    BOOST_CHECK(CheckFinalTx(tx));
    BOOST_CHECK(!CheckFinalTx(tx2));

    state = CValidationState();
    BOOST_CHECK(res = AcceptToMemoryPool(mempool, state, tx, false, NULL, true, false));
    BOOST_CHECK_MESSAGE(res, state.GetRejectReason());

    state = CValidationState();
    BOOST_CHECK(!AcceptToMemoryPool(mempool, state, tx2, false, NULL, true, false));

    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 2);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1].GetHash() == tx.GetHash());

    delete pblocktemplate;
    mempool.clear();

    // Restore standardness rules to prior setting.
    fRequireStandard = old_fRequireStandard;

    BOOST_FOREACH(CTransaction *tx, txFirst)
        delete tx;

    fCheckpointsEnabled = true;
}

BOOST_AUTO_TEST_SUITE_END()
