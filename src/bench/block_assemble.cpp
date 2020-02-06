// Copyright (c) 2011-2019 The Bitcoin Core developers
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

#include <bench/bench.h>
#include <consensus/validation.h>
#include <crypto/sha256.h>
#include <script/standard.h>
#include <test/util.h>
#include <txmempool.h>
#include <validation.h>


#include <list>
#include <vector>

static void AssembleBlock(benchmark::State& state)
{
    const WitnessV0ScriptEntry op_true(0 /* version */, CScript() << OP_TRUE);
    CScriptWitness witness;
    witness.stack.push_back(op_true.m_script);
    witness.stack.emplace_back();

    const WitnessV0ShortHash witness_program = op_true.GetShortHash();

    const CScript SCRIPT_PUB{CScript(OP_0) << std::vector<unsigned char>{witness_program.begin(), witness_program.end()}};

    // Collect some loose transactions that spend the coinbases of our mined blocks
    constexpr size_t NUM_BLOCKS{200};
    std::array<CTransactionRef, NUM_BLOCKS - COINBASE_MATURITY + 1> txs;
    for (size_t b{0}; b < NUM_BLOCKS; ++b) {
        CMutableTransaction tx;
        const auto& cbdata = MineBlock(SCRIPT_PUB);
        tx.vin.push_back(cbdata.first);
        tx.vin.back().scriptWitness = witness;
        tx.vout.emplace_back(1337, SCRIPT_PUB);
        tx.lock_height = cbdata.second;
        if (NUM_BLOCKS - b >= COINBASE_MATURITY)
            txs.at(b) = MakeTransactionRef(tx);
    }
    {
        LOCK(::cs_main); // Required for ::AcceptToMemoryPool.

        for (const auto& txr : txs) {
            CValidationState state;
            bool ret{::AcceptToMemoryPool(::mempool, state, txr, nullptr /* pfMissingInputs */, nullptr /* plTxnReplaced */, false /* bypass_limits */, /* nAbsurdFee */ 0)};
            assert(ret);
        }
    }

    while (state.KeepRunning()) {
        PrepareBlock(SCRIPT_PUB);
    }
}

BENCHMARK(AssembleBlock, 700);
