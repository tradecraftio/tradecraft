// Copyright (c) 2012-2020 The Bitcoin Core developers
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
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <test/util/wallet.h>
#include <validationinterface.h>
#include <wallet/wallet.h>

#include <optional>

static void WalletBalance(benchmark::Bench& bench, const bool set_dirty, const bool add_watchonly, const bool add_mine)
{
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    const auto& ADDRESS_WATCHONLY = ADDRESS_FCRT1_UNSPENDABLE;

    CWallet wallet{test_setup->m_node.chain.get(), "", CreateMockWalletDatabase()};
    {
        wallet.SetupLegacyScriptPubKeyMan();
        if (wallet.LoadWallet() != DBErrors::LOAD_OK) assert(false);
    }
    auto handler = test_setup->m_node.chain->handleNotifications({&wallet, [](CWallet*) {}});

    const std::optional<std::string> address_mine{add_mine ? std::optional<std::string>{getnewaddress(wallet)} : std::nullopt};
    if (add_watchonly) importaddress(wallet, ADDRESS_WATCHONLY);

    for (int i = 0; i < 100; ++i) {
        generatetoaddress(test_setup->m_node, address_mine.value_or(ADDRESS_WATCHONLY));
        generatetoaddress(test_setup->m_node, ADDRESS_WATCHONLY);
    }
    SyncWithValidationInterfaceQueue();

    auto bal = wallet.GetBalance(); // Cache

    bench.run([&] {
        if (set_dirty) wallet.MarkDirty();
        bal = wallet.GetBalance();
        if (add_mine) assert(bal.m_mine_trusted > 0);
        if (add_watchonly) assert(bal.m_watchonly_trusted > 0);
    });
}

static void WalletBalanceDirty(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ true, /* add_watchonly */ true, /* add_mine */ true); }
static void WalletBalanceClean(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ false, /* add_watchonly */ true, /* add_mine */ true); }
static void WalletBalanceMine(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ false, /* add_watchonly */ false, /* add_mine */ true); }
static void WalletBalanceWatch(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ false, /* add_watchonly */ true, /* add_mine */ false); }

BENCHMARK(WalletBalanceDirty);
BENCHMARK(WalletBalanceClean);
BENCHMARK(WalletBalanceMine);
BENCHMARK(WalletBalanceWatch);
