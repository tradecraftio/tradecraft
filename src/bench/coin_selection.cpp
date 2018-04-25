// Copyright (c) 2012-2021 The Bitcoin Core developers
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
#include <interfaces/chain.h>
#include <node/context.h>
#include <wallet/coinselection.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>

#include <set>

using node::NodeContext;
using wallet::AttemptSelection;
using wallet::CInputCoin;
using wallet::COutput;
using wallet::CWallet;
using wallet::CWalletTx;
using wallet::CoinEligibilityFilter;
using wallet::CoinSelectionParams;
using wallet::CreateDummyWalletDatabase;
using wallet::OutputGroup;
using wallet::SelectCoinsBnB;
using wallet::TxStateInactive;

static void addCoin(const CAmount& nValue, const CWallet& wallet, std::vector<std::unique_ptr<CWalletTx>>& wtxs)
{
    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++; // so all transactions get different hashes
    tx.vout.resize(1);
    tx.vout[0].SetReferenceValue(nValue);
    tx.lock_height = 1;
    wtxs.push_back(std::make_unique<CWalletTx>(MakeTransactionRef(std::move(tx)), TxStateInactive{}));
}

// Simple benchmark for wallet coin selection. Note that it maybe be necessary
// to build up more complicated scenarios in order to get meaningful
// measurements of performance. From laanwj, "Wallet coin selection is probably
// the hardest, as you need a wider selection of scenarios, just testing the
// same one over and over isn't too useful. Generating random isn't useful
// either for measurements."
// (https://github.com/bitcoin/bitcoin/issues/7883#issuecomment-224807484)
static void CoinSelection(benchmark::Bench& bench)
{
    NodeContext node;
    auto chain = interfaces::MakeChain(node);
    CWallet wallet(chain.get(), "", gArgs, CreateDummyWalletDatabase());
    std::vector<std::unique_ptr<CWalletTx>> wtxs;
    LOCK(wallet.cs_wallet);

    // Add coins.
    for (int i = 0; i < 1000; ++i) {
        addCoin(1000 * COIN, wallet, wtxs);
    }
    addCoin(3 * COIN, wallet, wtxs);

    // Create coins
    std::vector<COutput> coins;
    for (const auto& wtx : wtxs) {
        coins.emplace_back(wallet, 1 /* atheight */, wtx->tx->GetPresentValueOfOutput(0 /* n */, 1 /* refheight */), *wtx, 0 /* iIn */, 6 * 24 /* nDepthIn */, true /* spendable */, true /* solvable */, true /* safe */);
    }

    const CoinEligibilityFilter filter_standard(1 /* refheight */, 1, 6, 0);
    const CoinSelectionParams coin_selection_params(/* change_output_size= */ 34,
                                                    /* change_spend_size= */ 148, /* effective_feerate= */ CFeeRate(0),
                                                    /* long_term_feerate= */ CFeeRate(0), /* discard_feerate= */ CFeeRate(0),
                                                    /* tx_noinputs_size= */ 0, /* avoid_partial= */ false);
    bench.run([&] {
        auto result = AttemptSelection(wallet, 1003 * COIN, filter_standard, coins, coin_selection_params);
        assert(result);
        assert(result->GetSelectedValue() == 1003 * COIN);
        assert(result->GetInputSet().size() == 2);
    });
}

typedef std::set<CInputCoin> CoinSet;

// Copied from src/wallet/test/coinselector_tests.cpp
static void add_coin(const CAmount& nValue, int nInput, std::vector<OutputGroup>& set)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].SetReferenceValue(nValue);
    tx.lock_height = 1;
    CInputCoin coin(1, nValue, MakeTransactionRef(tx), nInput);
    set.emplace_back();
    set.back().Insert(coin, 0, true, 0, 0, false);
}
// Copied from src/wallet/test/coinselector_tests.cpp
static CAmount make_hard_case(int utxos, std::vector<OutputGroup>& utxo_pool)
{
    utxo_pool.clear();
    CAmount target = 0;
    for (int i = 0; i < utxos; ++i) {
        target += (CAmount)1 << (utxos+i);
        add_coin((CAmount)1 << (utxos+i), 2*i, utxo_pool);
        add_coin(((CAmount)1 << (utxos+i)) + ((CAmount)1 << (utxos-1-i)), 2*i + 1, utxo_pool);
    }
    return target;
}

static void BnBExhaustion(benchmark::Bench& bench)
{
    // Setup
    std::vector<OutputGroup> utxo_pool;

    bench.run([&] {
        // Benchmark
        CAmount target = make_hard_case(17, utxo_pool);
        SelectCoinsBnB(utxo_pool, target, 0); // Should exhaust

        // Cleanup
        utxo_pool.clear();
    });
}

BENCHMARK(CoinSelection);
BENCHMARK(BnBExhaustion);
