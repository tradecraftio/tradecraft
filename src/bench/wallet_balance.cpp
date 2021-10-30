// Copyright (c) 2012-2019 The Bitcoin Core developers
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
#include <interfaces/chain.h>
#include <optional.h>
#include <test/util.h>
#include <validationinterface.h>
#include <wallet/wallet.h>

static void WalletBalance(benchmark::State& state, const bool set_dirty, const bool add_watchonly, const bool add_mine)
{
    const auto& ADDRESS_WATCHONLY = ADDRESS_BCRT1_UNSPENDABLE;

    std::unique_ptr<interfaces::Chain> chain = interfaces::MakeChain();
    CWallet wallet{chain.get(), WalletLocation(), WalletDatabase::CreateMock()};
    {
        bool first_run;
        if (wallet.LoadWallet(first_run) != DBErrors::LOAD_OK) assert(false);
        wallet.handleNotifications();
    }


    const Optional<std::string> address_mine{add_mine ? Optional<std::string>{getnewaddress(wallet)} : nullopt};
    if (add_watchonly) importaddress(wallet, ADDRESS_WATCHONLY);

    for (int i = 0; i < 100; ++i) {
        generatetoaddress(address_mine.get_value_or(ADDRESS_WATCHONLY));
        generatetoaddress(ADDRESS_WATCHONLY);
    }
    SyncWithValidationInterfaceQueue();

    auto bal = wallet.GetBalance(); // Cache

    while (state.KeepRunning()) {
        if (set_dirty) wallet.MarkDirty();
        bal = wallet.GetBalance();
        if (add_mine) assert(bal.m_mine_trusted > 0);
        if (add_watchonly) assert(bal.m_watchonly_trusted > 0);
    }
}

static void WalletBalanceDirty(benchmark::State& state) { WalletBalance(state, /* set_dirty */ true, /* add_watchonly */ true, /* add_mine */ true); }
static void WalletBalanceClean(benchmark::State& state) { WalletBalance(state, /* set_dirty */ false, /* add_watchonly */ true, /* add_mine */ true); }
static void WalletBalanceMine(benchmark::State& state) { WalletBalance(state, /* set_dirty */ false, /* add_watchonly */ false, /* add_mine */ true); }
static void WalletBalanceWatch(benchmark::State& state) { WalletBalance(state, /* set_dirty */ false, /* add_watchonly */ true, /* add_mine */ false); }

BENCHMARK(WalletBalanceDirty, 2500);
BENCHMARK(WalletBalanceClean, 8000);
BENCHMARK(WalletBalanceMine, 16000);
BENCHMARK(WalletBalanceWatch, 8000);
