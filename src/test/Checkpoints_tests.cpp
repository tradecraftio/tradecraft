// Copyright (c) 2011-2013 The Bitcoin Core developers
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

//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "uint256.h"
#include "test/test_freicoin.h"
#include "chainparams.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(Checkpoints_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(sanity)
{
    const Checkpoints::CCheckpointData& checkpoints = Params(CBaseChainParams::MAIN).Checkpoints();
    uint256 p10080 = uint256S("0x00000000003ff9c4b806639ec4376cc9acafcdded0e18e9dbcc2fc42e8e72331");
    uint256 p28336 = uint256S("0x000000000000cc374a984c0deec9aed6fff764918e2cfd4be6670dd4d5292ccb");
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 10080, p10080));
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 28336, p28336));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 10080, p28336));
    BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 28336, p10080));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 10080+1, p28336));
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 28336+1, p10080));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate(checkpoints) >= 28336);
}    

BOOST_AUTO_TEST_SUITE_END()
