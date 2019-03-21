// Copyright (c) 2012-2015 The Bitcoin Core developers
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

#include "streams.h"
#include "support/allocators/zeroafterfree.h"
#include "test/test_bitcoin.h"

#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>
                    
using namespace std;
using namespace boost::assign; // bring 'operator+=()' into scope

BOOST_FIXTURE_TEST_SUITE(streams_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(streams_serializedata_xor)
{
    std::vector<char> in;
    std::vector<char> expected_xor;
    std::vector<unsigned char> key;
    CDataStream ds(in, 0, 0);

    // Degenerate case
    
    key += '\x00','\x00';
    ds.Xor(key);
    BOOST_CHECK_EQUAL(
            std::string(expected_xor.begin(), expected_xor.end()), 
            std::string(ds.begin(), ds.end()));

    in += '\x0f','\xf0';
    expected_xor += '\xf0','\x0f';
    
    // Single character key

    ds.clear();
    ds.insert(ds.begin(), in.begin(), in.end());
    key.clear();

    key += '\xff';
    ds.Xor(key);
    BOOST_CHECK_EQUAL(
            std::string(expected_xor.begin(), expected_xor.end()), 
            std::string(ds.begin(), ds.end())); 
    
    // Multi character key

    in.clear();
    expected_xor.clear();
    in += '\xf0','\x0f';
    expected_xor += '\x0f','\x00';
                        
    ds.clear();
    ds.insert(ds.begin(), in.begin(), in.end());

    key.clear();
    key += '\xff','\x0f';

    ds.Xor(key);
    BOOST_CHECK_EQUAL(
            std::string(expected_xor.begin(), expected_xor.end()), 
            std::string(ds.begin(), ds.end()));  
}         

BOOST_AUTO_TEST_SUITE_END()
