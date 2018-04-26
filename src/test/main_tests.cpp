// Copyright (c) 2014-2016 The Bitcoin Core developers
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

#include "chainparams.h"
#include "validation.h"
#include "net.h"

#include "test/test_freicoin.h"

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

static void TestBlockSubsidyHalvings(const Consensus::Params& consensusParams)
{
    int maxHalvings = 64;
    CAmount nInitialSubsidy = 50 * COIN;

    CAmount nPreviousSubsidy = nInitialSubsidy;
    for (int nHalvings = 1; nHalvings < maxHalvings; nHalvings++) {
        int nHeight = nHalvings * consensusParams.nSubsidyHalvingInterval;
        CAmount nSubsidy = GetBlockSubsidy(nHeight-1, consensusParams);
        BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy);
        nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
        BOOST_CHECK(nSubsidy <= nInitialSubsidy);
        BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
        nPreviousSubsidy = nSubsidy;
    }
    BOOST_CHECK_EQUAL(GetBlockSubsidy(maxHalvings * consensusParams.nSubsidyHalvingInterval, consensusParams), 0);
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
    Consensus::Params consensusParams(Params(CBaseChainParams::REGTEST).GetConsensus());
    consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    TestBlockSubsidyHalvings(consensusParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    TestBlockSubsidyHalvings(Params(CBaseChainParams::REGTEST).GetConsensus());
    TestBlockSubsidyHalvings(150); // As in regtest
    TestBlockSubsidyHalvings(1000); // Just another interval
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const Consensus::Params& consensusParams = Params(CBaseChainParams::MAIN).GetConsensus();
    CAmount nSum = 0;
    for (int nHeight = 0; nHeight < consensusParams.equilibrium_height; ++nHeight) {
        CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
        BOOST_CHECK(nSubsidy <= 75056846172LL);
        BOOST_CHECK(nSubsidy >=  9536743164LL);
        nSum += GetTimeAdjustedValue(nSubsidy, consensusParams.equilibrium_height-nHeight);
        BOOST_CHECK(nSum <= 9999990463180220LL);
    }
    BOOST_CHECK(nSum == 9999990463180220LL);
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test_bitcoin_mode)
{
    const Consensus::Params& consensusParams = Params(CBaseChainParams::REGTEST).GetConsensus();
    bool old_disable_time_adjust = disable_time_adjust;
    disable_time_adjust = true;
    try {
        CAmount nSum = 0;
        for (int nHeight = 1; nHeight < 10000; ++nHeight) {
            CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
            BOOST_CHECK(nSubsidy == ((int64_t)5000000000LL >> std::min(nHeight/150, 63)));
            nSum += GetTimeAdjustedValue(nSubsidy, 10000-nHeight);
            BOOST_CHECK(nSum <= 1494999998350LL);
        }
        BOOST_CHECK(nSum == 1494999998350LL);
    } catch (...) {
        disable_time_adjust = old_disable_time_adjust;
        throw;
    }
    disable_time_adjust = old_disable_time_adjust;
}

bool ReturnFalse() { return false; }
bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool (), CombinerAll> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}
BOOST_AUTO_TEST_SUITE_END()
