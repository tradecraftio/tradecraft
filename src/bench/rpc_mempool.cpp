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
#include <rpc/blockchain.h>
#include <txmempool.h>

#include <univalue.h>

#include <list>
#include <vector>

static void AddTx(const CTransactionRef& tx, const CAmount& fee, CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    LockPoints lp;
    pool.addUnchecked(CTxMemPoolEntry(tx, fee, /* time */ 0, /* height */ 1, /* spendsCoinbase */ false, /* sigOpCost */ 4, lp));
}

static void RpcMempool(benchmark::State& state)
{
    CTxMemPool pool;
    LOCK2(cs_main, pool.cs);

    for (int i = 0; i < 1000; ++i) {
        CMutableTransaction tx = CMutableTransaction();
        tx.vin.resize(1);
        tx.vin[0].scriptSig = CScript() << OP_1;
        tx.vin[0].scriptWitness.stack.push_back({1});
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
        tx.vout[0].SetReferenceValue(i);
        const CTransactionRef tx_r{MakeTransactionRef(tx)};
        AddTx(tx_r, /* fee */ i, pool);
    }

    while (state.KeepRunning()) {
        (void)MempoolToJSON(pool, /*verbose*/ true);
    }
}

BENCHMARK(RpcMempool, 40);
