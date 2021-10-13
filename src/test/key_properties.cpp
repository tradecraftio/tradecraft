// Copyright (c) 2018 The Bitcoin Core developers
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
#include <key.h>

#include <base58.h>
#include <script/script.h>
#include <uint256.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <test/test_bitcoin.h>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/Gen.h>

#include <test/gen/crypto_gen.h>

BOOST_FIXTURE_TEST_SUITE(key_properties, BasicTestingSetup)

/** Check CKey uniqueness */
RC_BOOST_PROP(key_uniqueness, (const CKey& key1, const CKey& key2))
{
    RC_ASSERT(!(key1 == key2));
}

/** Verify that a private key generates the correct public key */
RC_BOOST_PROP(key_generates_correct_pubkey, (const CKey& key))
{
    CPubKey pubKey = key.GetPubKey();
    RC_ASSERT(key.VerifyPubKey(pubKey));
}

/** Create a CKey using the 'Set' function must give us the same key */
RC_BOOST_PROP(key_set_symmetry, (const CKey& key))
{
    CKey key1;
    key1.Set(key.begin(), key.end(), key.IsCompressed());
    RC_ASSERT(key1 == key);
}

/** Create a CKey, sign a piece of data, then verify it with the public key */
RC_BOOST_PROP(key_sign_symmetry, (const CKey& key, const uint256& hash))
{
    std::vector<unsigned char> vchSig;
    key.Sign(hash, vchSig, 0);
    const CPubKey& pubKey = key.GetPubKey();
    RC_ASSERT(pubKey.Verify(hash, vchSig));
}
BOOST_AUTO_TEST_SUITE_END()
