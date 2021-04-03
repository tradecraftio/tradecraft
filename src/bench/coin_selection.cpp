// Copyright (c) 2012-2016 The Bitcoin Core developers
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

#include "bench.h"
#include "wallet/wallet.h"

#include <boost/foreach.hpp>
#include <set>

static void addCoin(const CAmount& nValue, int32_t refheight, const CWallet& wallet, std::vector<COutput>& vCoins)
{
    int nInput = 0;

    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++; // so all transactions get different hashes
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    tx.lock_height = refheight;
    CWalletTx* wtx = new CWalletTx(&wallet, MakeTransactionRef(std::move(tx)));

    int nAge = 6 * 24;
    COutput output(wtx, nInput, nAge, true, true);
    vCoins.push_back(output);
}

// Simple benchmark for wallet coin selection. Note that it maybe be necessary
// to build up more complicated scenarios in order to get meaningful
// measurements of performance. From laanwj, "Wallet coin selection is probably
// the hardest, as you need a wider selection of scenarios, just testing the
// same one over and over isn't too useful. Generating random isn't useful
// either for measurements."
// (https://github.com/bitcoin/bitcoin/issues/7883#issuecomment-224807484)
static void CoinSelection(benchmark::State& state)
{
    const CWallet wallet;
    std::vector<COutput> vCoins;
    LOCK(wallet.cs_wallet);

    while (state.KeepRunning()) {
        // Empty wallet.
        BOOST_FOREACH (COutput output, vCoins)
            delete output.tx;
        vCoins.clear();

        // Add coins.
        for (int i = 0; i < 1000; i++)
            addCoin(1000 * COIN, 0, wallet, vCoins);
        addCoin(3 * COIN, 0, wallet, vCoins);

        std::set<std::pair<const CWalletTx*, unsigned int> > setCoinsRet;
        CAmount nValueRet;
        bool success = wallet.SelectCoinsMinConf(1003 * COIN, 0, 1, 6, 0, vCoins, setCoinsRet, nValueRet);
        assert(success);
        assert(nValueRet == 1003 * COIN);
        assert(setCoinsRet.size() == 2);
    }
}

BENCHMARK(CoinSelection);
