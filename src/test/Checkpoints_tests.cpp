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
#include "test/test_bitcoin.h"
#include "chainparams.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(Checkpoints_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(sanity)
{
    const Checkpoints::CCheckpointData& checkpoints = Params(CBaseChainParams::MAIN).Checkpoints();
    uint256 p11111 = uint256S("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d");
    uint256 p134444 = uint256S("0x00000000000005b12ffd4cd315cd34ffd4a594f430ac814c91184a0d42d2b0fe");
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 11111, p11111));
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 134444, p134444));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 11111, p134444));
    BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 134444, p11111));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 11111+1, p134444));
    BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 134444+1, p11111));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate(checkpoints) >= 134444);
}    

BOOST_AUTO_TEST_SUITE_END()
