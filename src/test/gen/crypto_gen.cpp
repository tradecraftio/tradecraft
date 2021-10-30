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
#include <test/gen/crypto_gen.h>

#include <key.h>

#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Predicate.h>
#include <rapidcheck/gen/Container.h>

/** Generates 1 to 20 keys for OP_CHECKMULTISIG */
rc::Gen<std::vector<CKey>> MultisigKeys()
{
    return rc::gen::suchThat(rc::gen::arbitrary<std::vector<CKey>>(), [](const std::vector<CKey>& keys) {
        return keys.size() >= 1 && keys.size() <= 15;
    });
};
