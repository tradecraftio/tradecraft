// Copyright (c) 2022 The Bitcoin Core developers
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

#include <test/util/txmempool.h>

#include <chainparams.h>
#include <node/context.h>
#include <node/mempool_args.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/time.h>
#include <util/translation.h>

using node::NodeContext;

CTxMemPool::Options MemPoolOptionsForTest(const NodeContext& node)
{
    CTxMemPool::Options mempool_opts{
        .estimator = node.fee_estimator.get(),
        // Default to always checking mempool regardless of
        // chainparams.DefaultConsistencyChecks for tests
        .check_ratio = 1,
    };
    const auto err{ApplyArgsManOptions(*node.args, ::Params(), mempool_opts)};
    Assert(!err);
    return mempool_opts;
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CMutableTransaction& tx) const
{
    return FromTx(MakeTransactionRef(tx));
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CTransactionRef& tx) const
{
    return CTxMemPoolEntry{tx, nFee, TicksSinceEpoch<std::chrono::seconds>(time), nHeight, spendsCoinbase, sigOpCost, lp};
}
