// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <node/miner.h>
#include <policy/policy.h>
#include <script/standard.h>
#include <timedata.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/time.h>
#include <validation.h>
#include <versionbits.h>

#include <memory>

#include <boost/test/unit_test.hpp>

using node::BlockAssembler;
using node::CBlockTemplate;

namespace miner_tests {
struct MinerTestingSetup : public TestingSetup {
    void TestPackageSelection(const CChainParams& chainparams, const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, m_node.mempool->cs);
    void TestBasicMining(const CChainParams& chainparams, const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst, int baseheight) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, m_node.mempool->cs);
    void TestPrioritisedMining(const CChainParams& chainparams, const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, m_node.mempool->cs);
    bool TestSequenceLocks(const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, m_node.mempool->cs)
    {
        CCoinsViewMemPool view_mempool(&m_node.chainman->ActiveChainstate().CoinsTip(), *m_node.mempool);
        return CheckSequenceLocksAtTip(m_node.chainman->ActiveChain().Tip(), view_mempool, tx);
    }
    BlockAssembler AssemblerForTest(const CChainParams& params);
};
} // namespace miner_tests

BOOST_FIXTURE_TEST_SUITE(miner_tests, MinerTestingSetup)

static CFeeRate blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

BlockAssembler MinerTestingSetup::AssemblerForTest(const CChainParams& params)
{
    BlockAssembler::Options options;

    options.nBlockMaxWeight = MAX_BLOCK_WEIGHT;
    options.blockMinFeeRate = blockMinFeeRate;
    return BlockAssembler{m_node.chainman->ActiveChainstate(), m_node.mempool.get(), options};
}

constexpr static struct {
    unsigned char extranonce;
    unsigned int nonce;
} BLOCKINFO[]{
    {2, 0x1ad89ecd}, {0, 0x767229a5}, {0, 0x33363e69}, {0, 0x5899c287},
    {0, 0x28e4e292}, {1, 0x29de1388}, {0, 0xd63ec352}, {0, 0x0ed1c5dd},
    {0, 0x68eb5678}, {6, 0xf5814918}, {0, 0x7e8dcb1c}, {2, 0x0e517ed6},
    {0, 0xc8d0b1d0}, {0, 0x2a98794b}, {0, 0x1b313abd}, {0, 0x59d860aa},
    {0, 0x7a37bd60}, {0, 0x00b16dd1}, {0, 0x3ccc05c2}, {0, 0xa18ca381},
    {2, 0x03bb64ee}, {0, 0x803e4e97}, {0, 0xfa9dc745}, {0, 0x36f15d3a},
    {0, 0x47ee7c1d}, {2, 0x785e7cd4}, {1, 0x11922a08}, {1, 0x4e75efb4},
    {0, 0x4fa61751}, {0, 0x48c66bde}, {1, 0x3cbb64b2}, {0, 0x67b7798f},
    {3, 0x62d584ca}, {0, 0x1d5d7975}, {0, 0xb6ad20da}, {3, 0xc1d870aa},
    {2, 0x207cd3bf}, {0, 0x0655fcd0}, {0, 0x1e163d53}, {2, 0x9815fced},
    {0, 0x4e3002af}, {0, 0x2de1eef0}, {1, 0xae1a1bc8}, {2, 0x5d2afdd2},
    {8, 0x775f2539}, {1, 0xa0b823d4}, {0, 0x287fec20}, {0, 0x5914c6a4},
    {0, 0xd37a8e98}, {1, 0x10947313}, {2, 0xd7ba2816}, {1, 0x348327c0},
    {0, 0x11c52cb9}, {0, 0x080e1988}, {0, 0x4a562bcd}, {1, 0x91b7a9c5},
    {0, 0x1485c139}, {0, 0x47a7f898}, {4, 0x6da88be5}, {0, 0xedd02105},
    {1, 0xb4ec710f}, {0, 0xc71d1bdc}, {0, 0x630317be}, {2, 0x32385750},
    {0, 0x2a7e48d2}, {0, 0x01a39d61}, {1, 0xa10b3af8}, {0, 0x5ea85143},
    {0, 0x218146ce}, {0, 0x4b4e2448}, {0, 0x4c23e630}, {0, 0xb39ee3ec},
    {0, 0x6ef23559}, {0, 0xf68cebb5}, {0, 0x22ba6842}, {0, 0xa4e0228a},
    {1, 0x08eb1d0d}, {1, 0x263924eb}, {1, 0x09f64437}, {0, 0xafcebd03},
    {0, 0x178be1c9}, {1, 0xa66ecc8d}, {1, 0x237405a1}, {2, 0x4909e6b9},
    {0, 0x069ffa65}, {0, 0x509db10f}, {0, 0xd5cd4b60}, {1, 0x24c7e45b},
    {1, 0x2cb51358}, {1, 0x55787d31}, {0, 0x451796af}, {1, 0x5a06eb50},
    {0, 0x4067f679}, {1, 0xe5c4addd}, {0, 0x15b9a5d7}, {0, 0x83f49b2e},
    {1, 0xb773bd75}, {0, 0x21b6987d}, {0, 0xacd05a6a}, {2, 0xa15946a6},
    {1, 0x4606bf39}, {3, 0x8dd43bc5}, {1, 0xe529238c}, {2, 0x595b8855},
    {2, 0x140a7583}, {0, 0xeb84a300}, {2, 0xb983104d}, {0, 0x3b29b547},
    {1, 0x193a05aa}, {3, 0x50dba7d4}
};

static CBlockIndex CreateBlockIndex(int nHeight, CBlockIndex* active_chain_tip) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    CBlockIndex index;
    index.nHeight = nHeight;
    index.pprev = active_chain_tip;
    return index;
}

// Test suite for ancestor feerate transaction selection.
// Implemented as an additional function, rather than a separate test case,
// to allow reusing the blockchain created in CreateNewBlock_validity.
void MinerTestingSetup::TestPackageSelection(const CChainParams& chainparams, const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst)
{
    // Test the ancestor feerate transaction selection.
    TestMemPoolEntryHelper entry;

    // Test that a medium fee transaction will be selected after a higher fee
    // rate package with a low fee rate parent.
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 5000000000LL - 1000;
    tx.lock_height = txFirst.back()->lock_height;
    // This tx has a low fee: 1000 kria
    uint256 hashParentTx = tx.GetHash(); // save this txid for later use
    m_node.mempool->addUnchecked(entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a medium fee: 10000 kria
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = 5000000000LL - 10000;
    uint256 hashMediumFeeTx = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(10000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a high fee, but depends on the first transaction
    tx.vin[0].prevout.hash = hashParentTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000; // 50k kria fee
    uint256 hashHighFeeTx = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(50000).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));

    std::unique_ptr<CBlockTemplate> pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_REQUIRE_EQUAL(pblocktemplate->block.vtx.size(), 4U);
    BOOST_CHECK(pblocktemplate->block.vtx[1]->GetHash() == hashParentTx);
    BOOST_CHECK(pblocktemplate->block.vtx[2]->GetHash() == hashHighFeeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[3]->GetHash() == hashMediumFeeTx);

    // Test that a package below the block min tx fee doesn't get included
    tx.vin[0].prevout.hash = hashHighFeeTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000; // 0 fee
    uint256 hashFreeTx = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(0).FromTx(tx));
    size_t freeTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);

    // Calculate a fee on child transaction that will put the package just
    // below the block min tx fee (assuming 1 child tx of the same size).
    CAmount feeToUse = blockMinFeeRate.GetFee(2*freeTxSize) - 1;

    tx.vin[0].prevout.hash = hashFreeTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000 - feeToUse;
    uint256 hashLowFeeTx = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(feeToUse).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    // Verify that the free tx and the low fee tx didn't get selected
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx);
    }

    // Test that packages above the min relay fee do get included, even if one
    // of the transactions is below the min relay fee
    // Remove the low fee transaction and replace with a higher fee transaction
    m_node.mempool->removeRecursive(CTransaction(tx), MemPoolRemovalReason::REPLACED);
    tx.vout[0].nValue -= 2; // Now we should be just over the min relay fee
    hashLowFeeTx = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(feeToUse+2).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_REQUIRE_EQUAL(pblocktemplate->block.vtx.size(), 6U);
    BOOST_CHECK(pblocktemplate->block.vtx[4]->GetHash() == hashFreeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[5]->GetHash() == hashLowFeeTx);

    // Test that transaction selection properly updates ancestor fee
    // calculations as ancestor transactions get included in a block.
    // Add a 0-fee transaction that has 2 outputs.
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vout.resize(2);
    tx.vout[0].nValue = 5000000000LL - 100000000;
    tx.vout[1].nValue = 100000000; // 1FRC output
    uint256 hashFreeTx2 = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(0).SpendsCoinbase(true).FromTx(tx));

    // This tx can't be mined by itself
    tx.vin[0].prevout.hash = hashFreeTx2;
    tx.vout.resize(1);
    feeToUse = blockMinFeeRate.GetFee(freeTxSize);
    tx.vout[0].nValue = 5000000000LL - 100000000 - feeToUse;
    uint256 hashLowFeeTx2 = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(feeToUse).SpendsCoinbase(false).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);

    // Verify that this tx isn't selected.
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx2);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx2);
    }

    // This tx will be mineable, and should cause hashLowFeeTx2 to be selected
    // as well.
    tx.vin[0].prevout.n = 1;
    tx.vout[0].nValue = 100000000 - 10000; // 10k kria fee
    m_node.mempool->addUnchecked(entry.Fee(10000).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_REQUIRE_EQUAL(pblocktemplate->block.vtx.size(), 9U);
    BOOST_CHECK(pblocktemplate->block.vtx[8]->GetHash() == hashLowFeeTx2);
}

void MinerTestingSetup::TestBasicMining(const CChainParams& chainparams, const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst, int baseheight)
{
    uint256 hash;
    CMutableTransaction tx;
    TestMemPoolEntryHelper entry;
    entry.nFee = 11;
    entry.nHeight = 11;

    // Just to make sure we can still make simple blocks
    auto pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate);

    const CAmount BLOCKSUBSIDY = 50*COIN;
    const CAmount LOWFEE = CENT;
    const CAmount HIGHFEE = COIN;
    const CAmount HIGHERFEE = 4*COIN;

    tx.lock_height = txFirst.back()->lock_height;

    // block sigops > limit: 1000 CHECKMULTISIG + 1
    tx.vin.resize(1);
    // NOTE: OP_NOP is used to force 20 SigOps for the CHECKMULTISIG
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_0 << OP_0 << OP_NOP << OP_CHECKMULTISIG << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = BLOCKSUBSIDY;
    tx.lock_height = txFirst[0]->lock_height;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        // If we don't set the # of sig ops in the CTxMemPoolEntry, template creation fails
        m_node.mempool->addUnchecked(entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }

    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("bad-blk-sigops"));
    m_node.mempool->clear();

    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        // If we do set the # of sig ops in the CTxMemPoolEntry, template creation passes
        m_node.mempool->addUnchecked(entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).SigOpsCost(80).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    m_node.mempool->clear();

    // block size > limit
    tx.vin[0].scriptSig = CScript();
    // 18 * (520char + DROP) + OP_1 = 9433 bytes
    std::vector<unsigned char> vchData(520);
    for (unsigned int i = 0; i < 18; ++i)
        tx.vin[0].scriptSig << vchData << OP_DROP;
    tx.vin[0].scriptSig << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 128; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        m_node.mempool->addUnchecked(entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    m_node.mempool->clear();

    // orphan in *m_node.mempool, template creation fails
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(LOWFEE).Time(GetTime()).FromTx(tx));
    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("bad-txns-inputs-missingorspent"));
    m_node.mempool->clear();

    // child with higher feerate than parent
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    tx.lock_height = txFirst[1]->lock_height;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vin[0].prevout.hash = hash;
    tx.vin.resize(2);
    tx.vin[1].scriptSig = CScript() << OP_1;
    tx.vin[1].prevout.hash = txFirst[0]->GetHash();
    tx.vin[1].prevout.n = 0;
    tx.vout[0].nValue = tx.vout[0].nValue + txFirst[0]->GetPresentValueOfOutput(0, tx.lock_height) - HIGHERFEE; //First txn output + fresh coinbase - new txn fee
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(HIGHERFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    m_node.mempool->clear();

    // coinbase in *m_node.mempool, template creation fails
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
    tx.vout[0].nValue = 0;
    hash = tx.GetHash();
    // give it a fee so it'll get mined
    m_node.mempool->addUnchecked(entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    // Should throw bad-cb-multiple
    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("bad-cb-multiple"));
    m_node.mempool->clear();

    // double spend txn pair in *m_node.mempool, template creation fails
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    tx.lock_height = txFirst[0]->lock_height;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vout[0].scriptPubKey = CScript() << OP_2;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("bad-txns-inputs-missingorspent"));
    m_node.mempool->clear();

    // subsidy changing
    int nHeight = m_node.chainman->ActiveChain().Height();
    // Create an actual 209999-long block chain (without valid blocks).
    while (m_node.chainman->ActiveChain().Tip()->nHeight < 209999) {
        CBlockIndex* prev = m_node.chainman->ActiveChain().Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(InsecureRand256());
        m_node.chainman->ActiveChainstate().CoinsTip().SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        m_node.chainman->ActiveChain().SetTip(*next);
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    // Extend to a 210000-long block chain.
    while (m_node.chainman->ActiveChain().Tip()->nHeight < 210000) {
        CBlockIndex* prev = m_node.chainman->ActiveChain().Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(InsecureRand256());
        m_node.chainman->ActiveChainstate().CoinsTip().SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        m_node.chainman->ActiveChain().SetTip(*next);
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    // invalid p2sh txn in *m_node.mempool, template creation fails
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = BLOCKSUBSIDY-LOWFEE;
    CScript script = CScript() << OP_0;
    tx.vout[0].scriptPubKey = GetScriptForDestination(ScriptHash(script));
    tx.lock_height = txFirst[0]->lock_height;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vin[0].prevout.hash = hash;
    tx.vin[0].scriptSig = CScript() << std::vector<unsigned char>(script.begin(), script.end());
    tx.vout[0].nValue -= LOWFEE;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    // Should throw block-validation-failed
    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("block-validation-failed"));
    m_node.mempool->clear();

    // Delete the dummy blocks again.
    while (m_node.chainman->ActiveChain().Tip()->nHeight > nHeight) {
        CBlockIndex* del = m_node.chainman->ActiveChain().Tip();
        m_node.chainman->ActiveChain().SetTip(*Assert(del->pprev));
        m_node.chainman->ActiveChainstate().CoinsTip().SetBestBlock(del->pprev->GetBlockHash());
        delete del->phashBlock;
        delete del;
    }

    // non-final txs in mempool
    SetMockTime(m_node.chainman->ActiveChain().Tip()->GetMedianTimePast() + 1);
    const int flags{LOCKTIME_VERIFY_SEQUENCE};
    // height map
    std::vector<int> prevheights;

    // relative height locked
    tx.nVersion = 2;
    tx.vin.resize(1);
    prevheights.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash(); // only 1 transaction
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].nSequence = m_node.chainman->ActiveChain().Tip()->nHeight + 1; // txFirst[0] is the 2nd block
    prevheights[0] = baseheight + 1;
    tx.vout.resize(1);
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    tx.nLockTime = 0;
    tx.lock_height = txFirst[0]->lock_height;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(CTransaction{tx})); // Sequence locks fail

    {
        CBlockIndex* active_chain_tip = m_node.chainman->ActiveChain().Tip();
        BOOST_CHECK(SequenceLocks(CTransaction(tx), flags, prevheights, CreateBlockIndex(active_chain_tip->nHeight + 2, active_chain_tip))); // Sequence locks pass on 2nd block
    }

    // relative time locked
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | (((m_node.chainman->ActiveChain().Tip()->GetMedianTimePast()+1-m_node.chainman->ActiveChain()[1]->GetMedianTimePast()) >> CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) + 1); // txFirst[1] is the 3rd block
    tx.lock_height = txFirst[1]->lock_height;
    prevheights[0] = baseheight + 2;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(CTransaction{tx})); // Sequence locks fail

    const int SEQUENCE_LOCK_TIME = 512; // Sequence locks pass 512 seconds later
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; ++i)
        m_node.chainman->ActiveChain().Tip()->GetAncestor(m_node.chainman->ActiveChain().Tip()->nHeight - i)->nTime += SEQUENCE_LOCK_TIME; // Trick the MedianTimePast
    {
        CBlockIndex* active_chain_tip = m_node.chainman->ActiveChain().Tip();
        BOOST_CHECK(SequenceLocks(CTransaction(tx), flags, prevheights, CreateBlockIndex(active_chain_tip->nHeight + 1, active_chain_tip)));
    }

    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; ++i) {
        CBlockIndex* ancestor{Assert(m_node.chainman->ActiveChain().Tip()->GetAncestor(m_node.chainman->ActiveChain().Tip()->nHeight - i))};
        ancestor->nTime -= SEQUENCE_LOCK_TIME; // undo tricked MTP
    }

    // absolute height locked
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.lock_height = txFirst[2]->lock_height;
    tx.vin[0].nSequence = CTxIn::MAX_SEQUENCE_NONFINAL;
    prevheights[0] = baseheight + 3;
    tx.nLockTime = m_node.chainman->ActiveChain().Tip()->nHeight + 1;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(CTransaction{tx})); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(CTransaction(tx), m_node.chainman->ActiveChain().Tip()->nHeight + 2, m_node.chainman->ActiveChain().Tip()->GetMedianTimePast())); // Locktime passes on 2nd block

    // absolute time locked
    tx.vin[0].prevout.hash = txFirst[3]->GetHash();
    tx.nLockTime = m_node.chainman->ActiveChain().Tip()->GetMedianTimePast();
    tx.lock_height = txFirst[3]->lock_height;
    prevheights.resize(1);
    prevheights[0] = baseheight + 4;
    hash = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(CTransaction{tx})); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(CTransaction(tx), m_node.chainman->ActiveChain().Tip()->nHeight + 2, m_node.chainman->ActiveChain().Tip()->GetMedianTimePast() + 1)); // Locktime passes 1 second later

    // mempool-dependent transactions (not added)
    tx.vin[0].prevout.hash = hash;
    prevheights[0] = m_node.chainman->ActiveChain().Tip()->nHeight + 1;
    tx.nLockTime = 0;
    tx.vin[0].nSequence = 0;
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime passes
    BOOST_CHECK(TestSequenceLocks(CTransaction{tx})); // Sequence locks pass
    tx.vin[0].nSequence = 1;
    BOOST_CHECK(!TestSequenceLocks(CTransaction{tx})); // Sequence locks fail
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG;
    BOOST_CHECK(TestSequenceLocks(CTransaction{tx})); // Sequence locks pass
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | 1;
    BOOST_CHECK(!TestSequenceLocks(CTransaction{tx})); // Sequence locks fail

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    // None of the of the absolute height/time locked tx should have made
    // it into the template because we still check IsFinalTx in CreateNewBlock,
    // but relative locked txs will if inconsistently added to mempool.
    // For now these will still generate a valid template until BIP68 soft fork
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3U);
    // However if we advance height by 1 and time by SEQUENCE_LOCK_TIME, all of them should be mined
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; ++i) {
        CBlockIndex* ancestor{Assert(m_node.chainman->ActiveChain().Tip()->GetAncestor(m_node.chainman->ActiveChain().Tip()->nHeight - i))};
        ancestor->nTime += SEQUENCE_LOCK_TIME; // Trick the MedianTimePast
    }
    m_node.chainman->ActiveChain().Tip()->nHeight++;
    SetMockTime(m_node.chainman->ActiveChain().Tip()->GetMedianTimePast() + 1);

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 5U);
}

void MinerTestingSetup::TestPrioritisedMining(const CChainParams& chainparams, const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst)
{
    TestMemPoolEntryHelper entry;

    // Test that a tx below min fee but prioritised is included
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout.resize(1);
    tx.vout[0].nValue = 5000000000LL; // 0 fee
    tx.lock_height = txFirst[0]->lock_height;
    uint256 hashFreePrioritisedTx = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(0).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    m_node.mempool->PrioritiseTransaction(hashFreePrioritisedTx, 5 * COIN);

    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout[0].nValue = 5000000000LL - 1000;
    tx.lock_height = txFirst[1]->lock_height;
    // This tx has a low fee: 1000 kria
    uint256 hashParentTx = tx.GetHash(); // save this txid for later use
    m_node.mempool->addUnchecked(entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a medium fee: 10000 kria
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vout[0].nValue = 5000000000LL - 10000;
    tx.lock_height = txFirst[2]->lock_height;
    uint256 hashMediumFeeTx = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(10000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    m_node.mempool->PrioritiseTransaction(hashMediumFeeTx, -5 * COIN);

    // This tx also has a low fee, but is prioritised
    tx.vin[0].prevout.hash = hashParentTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 1000; // 1000 kria fee
    tx.lock_height = txFirst[1]->lock_height;
    uint256 hashPrioritsedChild = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(1000).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    m_node.mempool->PrioritiseTransaction(hashPrioritsedChild, 2 * COIN);

    // Test that transaction selection properly updates ancestor fee calculations as prioritised
    // parents get included in a block. Create a transaction with two prioritised ancestors, each
    // included by itself: FreeParent <- FreeChild <- FreeGrandchild.
    // When FreeParent is added, a modified entry will be created for FreeChild + FreeGrandchild
    // FreeParent's prioritisation should not be included in that entry.
    // When FreeChild is included, FreeChild's prioritisation should also not be included.
    tx.vin[0].prevout.hash = txFirst[3]->GetHash();
    tx.vout[0].nValue = 5000000000LL; // 0 fee
    tx.lock_height = txFirst[3]->lock_height;
    uint256 hashFreeParent = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(0).SpendsCoinbase(true).FromTx(tx));
    m_node.mempool->PrioritiseTransaction(hashFreeParent, 10 * COIN);

    tx.vin[0].prevout.hash = hashFreeParent;
    tx.vout[0].nValue = 5000000000LL; // 0 fee
    uint256 hashFreeChild = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(0).SpendsCoinbase(false).FromTx(tx));
    m_node.mempool->PrioritiseTransaction(hashFreeChild, 1 * COIN);

    tx.vin[0].prevout.hash = hashFreeChild;
    tx.vout[0].nValue = 5000000000LL; // 0 fee
    uint256 hashFreeGrandchild = tx.GetHash();
    m_node.mempool->addUnchecked(entry.Fee(0).SpendsCoinbase(false).FromTx(tx));

    auto pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_REQUIRE_EQUAL(pblocktemplate->block.vtx.size(), 6U);
    BOOST_CHECK(pblocktemplate->block.vtx[1]->GetHash() == hashFreeParent);
    BOOST_CHECK(pblocktemplate->block.vtx[2]->GetHash() == hashFreePrioritisedTx);
    BOOST_CHECK(pblocktemplate->block.vtx[3]->GetHash() == hashParentTx);
    BOOST_CHECK(pblocktemplate->block.vtx[4]->GetHash() == hashPrioritsedChild);
    BOOST_CHECK(pblocktemplate->block.vtx[5]->GetHash() == hashFreeChild);
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        // The FreeParent and FreeChild's prioritisations should not impact the child.
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeGrandchild);
        // De-prioritised transaction should not be included.
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashMediumFeeTx);
    }
}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    // Note that by default, these tests run with size accounting enabled.
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const CChainParams& chainparams = *chainParams;
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    std::unique_ptr<CBlockTemplate> pblocktemplate;

    fCheckpointsEnabled = false;

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    // We can't make transactions until we have inputs
    // Therefore, load 110 blocks :)
    static_assert(std::size(BLOCKINFO) == 110, "Should have 110 blocks to import");
    int baseheight = 0;
    std::vector<CTransactionRef> txFirst;
    for (const auto& bi : BLOCKINFO) {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        {
            LOCK(cs_main);
            pblock->nVersion = VERSIONBITS_TOP_BITS;
            pblock->nTime = m_node.chainman->ActiveChain().Tip()->GetMedianTimePast()+1;
            CMutableTransaction txCoinbase(*pblock->vtx[0]);
            txCoinbase.nVersion = 2;
            txCoinbase.vin[0].scriptSig = CScript{} << static_cast<int64_t>(m_node.chainman->ActiveChain().Height() + 1) << CScriptNum(bi.extranonce);
            txCoinbase.vout.resize(1); // Ignore the (optional) segwit commitment added by CreateNewBlock (as the hardcoded nonces don't account for this)
            txCoinbase.vout[0].nValue = 50 * COIN;
            txCoinbase.vout[0].scriptPubKey = CScript();
            txCoinbase.lock_height = m_node.chainman->ActiveChain().Height() + 1;
            pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
            if (txFirst.size() == 0)
                baseheight = m_node.chainman->ActiveChain().Height();
            if (txFirst.size() < 4)
                txFirst.push_back(pblock->vtx[0]);
            pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
            pblock->nNonce = bi.nonce;
        }
        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
        BOOST_CHECK(Assert(m_node.chainman)->ProcessNewBlock(shared_pblock, true, true, nullptr));
        pblock->hashPrevBlock = pblock->GetHash();
    }

    LOCK(cs_main);
    LOCK(m_node.mempool->cs);

    TestBasicMining(chainparams, scriptPubKey, txFirst, baseheight);

    m_node.chainman->ActiveChain().Tip()->nHeight--;
    SetMockTime(0);
    m_node.mempool->clear();

    // To get around standardness rules we use an OP_TRUE script
    // behind a P2SH construction, and turn off standardness so
    // P2SH redeem scripts aren't checked.
    CScript p2shTrue = CScript() << OP_TRUE;
    const auto old_require_standard = m_node.mempool->m_require_standard;
    *const_cast<bool*>(&m_node.mempool->m_require_standard) = false;

    // Test non-monotonic lock_height by creating two dependent
    // transactions where the second transaction has a lower
    // lock_height than the first. This shouldn't pass validation and
    // shouldn't make it into a block template.
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].nSequence = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 2500000000LL;
    tx.vout[0].scriptPubKey = GetScriptForDestination(ScriptHash(p2shTrue));
    tx.lock_height = m_node.chainman->ActiveChain().Tip()->nHeight + 1;
    uint256 hash = tx.GetHash();

    CMutableTransaction tx2;
    tx2.vin.resize(1);
    tx2.vin[0].prevout.hash = hash;
    tx2.vin[0].prevout.n = 0;
    tx2.vin[0].scriptSig = CScript() << std::vector<unsigned char>(p2shTrue.begin(), p2shTrue.end());
    tx2.vin[0].nSequence = 0;
    tx2.vout.resize(1);
    tx2.vout[0].nValue = 1250000000LL;
    tx2.vout[0].scriptPubKey = GetScriptForDestination(ScriptHash(p2shTrue));
    tx2.lock_height = m_node.chainman->ActiveChain().Tip()->nHeight;
    hash = tx2.GetHash();

    // Both transactions are final, which doesn't consider context
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction(tx)));
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction(tx2)));

    // But only the first transaction makes it into the mempool
    {
        CTransaction _tx(tx);
        const auto res = AcceptToMemoryPool(m_node.chainman->ActiveChainstate(), MakeTransactionRef(std::move(_tx)), GetTime(), /*bypass_limits=*/false, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(res.m_result_type == MempoolAcceptResult::ResultType::VALID, res.m_state.GetRejectReason());
    }

    {
        CTransaction _tx2(tx2);
        const auto res = AcceptToMemoryPool(m_node.chainman->ActiveChainstate(), MakeTransactionRef(std::move(_tx2)), GetTime(), /*bypass_limits=*/false, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(res.m_result_type == MempoolAcceptResult::ResultType::INVALID, res.m_state.GetRejectReason());
    }

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 2);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1]->GetHash() == tx.GetHash());

    // Now we try connecting the block to engage consensus code checks
    // on monotonic lock_heights.

    // The block with one transaction would be valid, if mined
    {
        bool res = false;
        BlockValidationState block_state;
        BOOST_CHECK(res = TestBlockValidity(block_state, chainparams, m_node.chainman->ActiveChainstate(), pblocktemplate->block, m_node.chainman->ActiveChain().Tip(), GetAdjustedTime, false, false));
        BOOST_CHECK_MESSAGE(res, block_state.GetRejectReason());
    }

    // But force inclusion of the second transaction, and it fails
    pblocktemplate->block.vtx.resize(3);
    {
        CTransaction _tx2(tx2);
        pblocktemplate->block.vtx[2] = MakeTransactionRef(std::move(_tx2));
        BlockValidationState block_state;
        BOOST_CHECK(!TestBlockValidity(block_state, chainparams, m_node.chainman->ActiveChainstate(), pblocktemplate->block, m_node.chainman->ActiveChain().Tip(), GetAdjustedTime, false, false));
        BOOST_CHECK_MESSAGE(block_state.GetRejectReason() == "bad-txns-non-monotonic-lock-height", block_state.GetRejectReason());
    }

    m_node.mempool->clear();

    // Change the lock_height to be the same and it works
    ++tx2.lock_height;
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction(tx)));
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction(tx2)));

    {
        CTransaction _tx(tx);
        const auto res = AcceptToMemoryPool(m_node.chainman->ActiveChainstate(), MakeTransactionRef(std::move(_tx)), GetTime(), /*bypass_limits=*/false, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(res.m_result_type == MempoolAcceptResult::ResultType::VALID, res.m_state.GetRejectReason());
    }

    {
        CTransaction _tx2(tx2);
        const auto res = AcceptToMemoryPool(m_node.chainman->ActiveChainstate(), MakeTransactionRef(std::move(_tx2)), GetTime(), /*bypass_limits=*/false, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(res.m_result_type == MempoolAcceptResult::ResultType::VALID, res.m_state.GetRejectReason());
    }

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1]->GetHash() == tx.GetHash());
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 3 && pblocktemplate->block.vtx[2]->GetHash() == tx2.GetHash());

    {
        bool res = false;
        BlockValidationState block_state;
        BOOST_CHECK(res = TestBlockValidity(block_state, chainparams, m_node.chainman->ActiveChainstate(), pblocktemplate->block, m_node.chainman->ActiveChain().Tip(), GetAdjustedTime, false, false));
        BOOST_CHECK_MESSAGE(res, block_state.GetRejectReason());
    }

    m_node.mempool->clear();

    // However a strictly increasing block height would run afoul of
    // the rule that lock_heights not exceed the current block height
    ++tx2.lock_height;
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction(tx)));
    BOOST_CHECK(!CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction(tx2)));

    {
        CTransaction _tx(tx);
        const auto res = AcceptToMemoryPool(m_node.chainman->ActiveChainstate(), MakeTransactionRef(std::move(_tx)), GetTime(), /*bypass_limits=*/false, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(res.m_result_type == MempoolAcceptResult::ResultType::VALID, res.m_state.GetRejectReason());
    }

    {
        CTransaction _tx2(tx2);
        const auto res = AcceptToMemoryPool(m_node.chainman->ActiveChainstate(), MakeTransactionRef(std::move(_tx2)), GetTime(), /*bypass_limits=*/false, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(res.m_result_type == MempoolAcceptResult::ResultType::INVALID, res.m_state.GetRejectReason());
    }

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 2);
    BOOST_CHECK(pblocktemplate->block.vtx.size() >= 2 && pblocktemplate->block.vtx[1]->GetHash() == tx.GetHash());

    m_node.mempool->clear();

    // Restore standardness rules to prior setting.
    *const_cast<bool*>(&m_node.mempool->m_require_standard) = old_require_standard;

    // TestPackageSelection features hand-crafted tests that are not
    // written in a way that is compatible with 5% demurrage.  So we
    // temporarily disable time-value adjustments.
    auto old_disable_time_adjust = disable_time_adjust;
    disable_time_adjust = true;
    TestPackageSelection(chainparams, scriptPubKey, txFirst);
    disable_time_adjust = old_disable_time_adjust;

    m_node.chainman->ActiveChain().Tip()->nHeight--;
    SetMockTime(0);
    m_node.mempool->clear();

    TestPrioritisedMining(chainparams, scriptPubKey, txFirst);

    fCheckpointsEnabled = true;
}

BOOST_AUTO_TEST_SUITE_END()
