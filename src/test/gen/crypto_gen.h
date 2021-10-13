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
#ifndef BITCOIN_TEST_GEN_CRYPTO_GEN_H
#define BITCOIN_TEST_GEN_CRYPTO_GEN_H

#include <key.h>
#include <random.h>
#include <uint256.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Create.h>
#include <rapidcheck/gen/Numeric.h>

/** Generates 1 to 15 keys for OP_CHECKMULTISIG */
rc::Gen<std::vector<CKey>> MultisigKeys();

namespace rc
{
/** Generator for a new CKey */
template <>
struct Arbitrary<CKey> {
    static Gen<CKey> arbitrary()
    {
        return rc::gen::map<int>([](int x) {
            CKey key;
            key.MakeNewKey(true);
            return key;
        });
    };
};

/** Generator for a CPrivKey */
template <>
struct Arbitrary<CPrivKey> {
    static Gen<CPrivKey> arbitrary()
    {
        return gen::map(gen::arbitrary<CKey>(), [](const CKey& key) {
            return key.GetPrivKey();
        });
    };
};

/** Generator for a new CPubKey */
template <>
struct Arbitrary<CPubKey> {
    static Gen<CPubKey> arbitrary()
    {
        return gen::map(gen::arbitrary<CKey>(), [](const CKey& key) {
            return key.GetPubKey();
        });
    };
};
/** Generates a arbitrary uint256 */
template <>
struct Arbitrary<uint256> {
    static Gen<uint256> arbitrary()
    {
        return rc::gen::just(GetRandHash());
    };
};
} //namespace rc
#endif
