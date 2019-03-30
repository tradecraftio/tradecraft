// Copyright (c) 2014-2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include "chainparams.h"
#include "main.h"

#include "test/test_freicoin.h"

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

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
        for (int nHeight = 1; nHeight <= 424242; ++nHeight) {
            CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
            BOOST_CHECK(nSubsidy == 5000000000LL);
            nSum += GetTimeAdjustedValue(nSubsidy, consensusParams.equilibrium_height-nHeight);
            BOOST_CHECK(nSum <= 2121210000000000LL);
        }
        BOOST_CHECK(nSum == 2121210000000000LL);
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
