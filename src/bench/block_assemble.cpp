// Copyright (c) 2011-2022 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <bench/bench.h>
#include <consensus/validation.h>
#include <crypto/sha256.h>
#include <node/miner.h>
#include <random.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <validation.h>


#include <vector>

static void AssembleBlock(benchmark::Bench& bench)
{
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    CScriptWitness witness;
    witness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    witness.stack.emplace_back();

    // Collect some loose transactions that spend the coinbases of our mined blocks
    constexpr size_t NUM_BLOCKS{200};
    std::array<CTransactionRef, NUM_BLOCKS - COINBASE_MATURITY + 1> txs;
    for (size_t b{0}; b < NUM_BLOCKS; ++b) {
        CMutableTransaction tx;
        const auto cbdata = MineBlock(test_setup->m_node, P2WSH_OP_TRUE);
        tx.vin.emplace_back(cbdata.first);
        tx.vin.back().scriptWitness = witness;
        tx.vout.emplace_back(1337, P2WSH_OP_TRUE);
        tx.lock_height = cbdata.second;
        if (NUM_BLOCKS - b >= COINBASE_MATURITY)
            txs.at(b) = MakeTransactionRef(tx);
    }
    {
        LOCK(::cs_main);

        for (const auto& txr : txs) {
            const MempoolAcceptResult res = test_setup->m_node.chainman->ProcessTransaction(txr);
            assert(res.m_result_type == MempoolAcceptResult::ResultType::VALID);
        }
    }

    bench.run([&] {
        PrepareBlock(test_setup->m_node, P2WSH_OP_TRUE);
    });
}
static void BlockAssemblerAddPackageTxns(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};
    auto testing_setup{MakeNoLogFileContext<TestChain100Setup>()};
    testing_setup->PopulateMempool(det_rand, /*num_transactions=*/1000, /*submit=*/true);
    node::BlockAssembler::Options assembler_options;
    assembler_options.test_block_validity = false;

    bench.run([&] {
        PrepareBlock(testing_setup->m_node, P2WSH_OP_TRUE, assembler_options);
    });
}

BENCHMARK(AssembleBlock, benchmark::PriorityLevel::HIGH);
BENCHMARK(BlockAssemblerAddPackageTxns, benchmark::PriorityLevel::LOW);
