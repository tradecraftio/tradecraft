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

#include <bench/bench.h>
#include <chainparamsbase.h>
#include <rpc/mempool.h>
#include <test/util/setup_common.h>
#include <txmempool.h>

#include <univalue.h>


static void AddTx(const CTransactionRef& tx, const CAmount& fee, CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    LockPoints lp;
    pool.addUnchecked(CTxMemPoolEntry(tx, fee, /*time=*/0, /*entry_height=*/1, /*spends_coinbase=*/false, /*sigops_cost=*/4, lp));
}

static void RpcMempool(benchmark::Bench& bench)
{
    const auto testing_setup = MakeNoLogFileContext<const ChainTestingSetup>(CBaseChainParams::MAIN);
    CTxMemPool& pool = *Assert(testing_setup->m_node.mempool);
    LOCK2(cs_main, pool.cs);

    for (int i = 0; i < 1000; ++i) {
        CMutableTransaction tx = CMutableTransaction();
        tx.vin.resize(1);
        tx.vin[0].scriptSig = CScript() << OP_1;
        tx.vin[0].scriptWitness.stack.push_back({1});
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
        tx.vout[0].nValue = i;
        const CTransactionRef tx_r{MakeTransactionRef(tx)};
        AddTx(tx_r, /*fee=*/i, pool);
    }

    bench.run([&] {
        (void)MempoolToJSON(pool, /*verbose=*/true);
    });
}

BENCHMARK(RpcMempool);
