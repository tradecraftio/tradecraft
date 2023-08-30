// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#include <node/mempool_args.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;
} // namespace

void initialize_rbf()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET_INIT(rbf, initialize_rbf)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
    if (!mtx) {
        return;
    }

    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node)};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        const std::optional<CMutableTransaction> another_mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
        if (!another_mtx) {
            break;
        }
        const CTransaction another_tx{*another_mtx};
        if (fuzzed_data_provider.ConsumeBool() && !mtx->vin.empty()) {
            mtx->vin[0].prevout = COutPoint{another_tx.GetHash(), 0};
        }
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(ConsumeTxMemPoolEntry(fuzzed_data_provider, another_tx));
    }
    const CTransaction tx{*mtx};
    if (fuzzed_data_provider.ConsumeBool()) {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(ConsumeTxMemPoolEntry(fuzzed_data_provider, tx));
    }
    {
        LOCK(pool.cs);
        (void)IsRBFOptIn(tx, pool);
    }
}
