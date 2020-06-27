// Copyright (c) 2018 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <boost/test/unit_test.hpp>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <miner.h>
#include <pow.h>
#include <random.h>
#include <test/test_freicoin.h>
#include <validation.h>
#include <validationinterface.h>

struct RegtestingSetup : public TestingSetup {
    RegtestingSetup() : TestingSetup(CBaseChainParams::REGTEST) {}
};

BOOST_FIXTURE_TEST_SUITE(validation_block_tests, RegtestingSetup)

struct TestSubscriber : public CValidationInterface {
    uint256 m_expected_tip;

    explicit TestSubscriber(uint256 tip) : m_expected_tip(tip) {}

    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, pindexNew->GetBlockHash());
    }

    void BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex, const std::vector<CTransactionRef>& txnConflicted) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, block->hashPrevBlock);
        BOOST_CHECK_EQUAL(m_expected_tip, pindex->pprev->GetBlockHash());

        m_expected_tip = block->GetHash();
    }

    void BlockDisconnected(const std::shared_ptr<const CBlock>& block) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, block->GetHash());

        m_expected_tip = block->hashPrevBlock;
    }
};

std::shared_ptr<CBlock> Block(const uint256& prev_hash, int height)
{
    static int i = 0;
    static uint64_t time = Params().GenesisBlock().nTime;

    CScript pubKey;
    pubKey << i++ << OP_TRUE;

    auto ptemplate = BlockAssembler(Params()).CreateNewBlock(pubKey);
    auto pblock = std::make_shared<CBlock>(ptemplate->block);
    pblock->hashPrevBlock = prev_hash;
    if (ptemplate->has_block_final_tx) {
        CMutableTransaction txBlockFinal(*pblock->vtx.back());
        txBlockFinal.lock_height = (uint32_t)height;
        pblock->vtx.back() = MakeTransactionRef(std::move(txBlockFinal));
    }
    pblock->nTime = ++time;

    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vout.resize(txCoinbase.vout.size() - 1);
    txCoinbase.vin[0].scriptWitness.SetNull();
    txCoinbase.lock_height = (uint32_t)height;
    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));

    return pblock;
}

std::shared_ptr<CBlock> FinalizeBlock(std::shared_ptr<CBlock> pblock)
{
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

    if (!pblock->m_aux_pow.IsNull()) {
        std::vector<unsigned char> extranonce;
        IncrementExtraNonceAux(*pblock, extranonce);
        while (!CheckAuxiliaryProofOfWork(*pblock, Params().GetConsensus())) {
            ++(pblock->m_aux_pow.m_aux_nonce);
        }
        const auto aux_hash2 = pblock->GetAuxiliaryHash(Params().GetConsensus()).second;
        CMutableTransaction txCoinbase(*pblock->vtx[0]);
        txCoinbase.vin[0].scriptSig.insert(txCoinbase.vin[0].scriptSig.end(), aux_hash2.begin(), aux_hash2.end());
        assert(txCoinbase.vin[0].scriptSig.size() <= 100);
        pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    }

    while (!CheckProofOfWork(*pblock, Params().GetConsensus())) {
        ++(pblock->nNonce);
    }

    return pblock;
}

// construct a valid block
const std::shared_ptr<const CBlock> GoodBlock(const uint256& prev_hash, int height)
{
    return FinalizeBlock(Block(prev_hash, height));
}

// construct an invalid block (but with a valid header)
const std::shared_ptr<const CBlock> BadBlock(const uint256& prev_hash, int height)
{
    auto pblock = Block(prev_hash, height);

    CMutableTransaction coinbase_spend;
    coinbase_spend.vin.push_back(CTxIn(COutPoint(pblock->vtx[0]->GetHash(), 0), CScript(), 0));
    coinbase_spend.vout.push_back(pblock->vtx[0]->vout[0]);

    CTransactionRef tx = MakeTransactionRef(coinbase_spend);
    pblock->vtx.push_back(tx);

    auto ret = FinalizeBlock(pblock);
    return ret;
}

void BuildChain(const uint256& root, int root_height, int height, const unsigned int invalid_rate, const unsigned int branch_rate, const unsigned int max_size, std::vector<std::shared_ptr<const CBlock>>& blocks)
{
    if (height <= 0 || blocks.size() >= max_size) return;
    int next_height = root_height + 1;

    bool gen_invalid = InsecureRandRange(100) < invalid_rate;
    bool gen_fork = InsecureRandRange(100) < branch_rate;

    const std::shared_ptr<const CBlock> pblock = gen_invalid ? BadBlock(root, next_height) : GoodBlock(root, next_height);
    blocks.push_back(pblock);
    if (!gen_invalid) {
        BuildChain(pblock->GetHash(), next_height, height - 1, invalid_rate, branch_rate, max_size, blocks);
    }

    if (gen_fork) {
        blocks.push_back(GoodBlock(root, next_height));
        BuildChain(blocks.back()->GetHash(), next_height, height - 1, invalid_rate, branch_rate, max_size, blocks);
    }
}

BOOST_AUTO_TEST_CASE(processnewblock_signals_ordering)
{
    // build a large-ish chain that's likely to have some forks
    std::vector<std::shared_ptr<const CBlock>> blocks;
    while (blocks.size() < 50) {
        blocks.clear();
        BuildChain(Params().GenesisBlock().GetHash(), 0, 100, 15, 10, 500, blocks);
    }

    bool ignored;
    CValidationState state;
    std::vector<CBlockHeader> headers;
    std::transform(blocks.begin(), blocks.end(), std::back_inserter(headers), [](std::shared_ptr<const CBlock> b) { return b->GetBlockHeader(); });

    // Process all the headers so we understand the toplogy of the chain
    BOOST_CHECK(ProcessNewBlockHeaders(headers, state, Params()));

    // Connect the genesis block and drain any outstanding events
    BOOST_CHECK(ProcessNewBlock(Params(), std::make_shared<CBlock>(Params().GenesisBlock()), true, &ignored));
    SyncWithValidationInterfaceQueue();

    // subscribe to events (this subscriber will validate event ordering)
    const CBlockIndex* initial_tip = nullptr;
    {
        LOCK(cs_main);
        initial_tip = chainActive.Tip();
    }
    TestSubscriber sub(initial_tip->GetBlockHash());
    RegisterValidationInterface(&sub);

    // create a bunch of threads that repeatedly process a block generated above at random
    // this will create parallelism and randomness inside validation - the ValidationInterface
    // will subscribe to events generated during block validation and assert on ordering invariance
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&blocks]() {
            bool ignored;
            FastRandomContext insecure;
            for (int i = 0; i < 1000; i++) {
                auto block = blocks[insecure.randrange(blocks.size() - 1)];
                ProcessNewBlock(Params(), block, true, &ignored);
            }

            // to make sure that eventually we process the full chain - do it here
            for (auto block : blocks) {
                if (block->vtx.size() == 1) {
                    bool processed = ProcessNewBlock(Params(), block, true, &ignored);
                    assert(processed);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
    while (GetMainSignals().CallbacksPending() > 0) {
        MilliSleep(100);
    }

    UnregisterValidationInterface(&sub);

    BOOST_CHECK_EQUAL(sub.m_expected_tip, chainActive.Tip()->GetBlockHash());
}

BOOST_AUTO_TEST_SUITE_END()
