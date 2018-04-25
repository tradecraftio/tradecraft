// Copyright (c) 2023 The Bitcoin Core developers
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

#include <test/util/coins.h>

#include <coins.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/random.h>
#include <uint256.h>

#include <stdint.h>
#include <utility>

COutPoint AddTestCoin(CCoinsViewCache& coins_view)
{
    Coin new_coin;
    const uint256 txid{InsecureRand256()};
    COutPoint outpoint{txid, /*nIn=*/0};
    new_coin.nHeight = 1;
    new_coin.out.SetReferenceValue(InsecureRandMoneyAmount());
    new_coin.out.scriptPubKey.assign(uint32_t{56}, 1);
    coins_view.AddCoin(outpoint, std::move(new_coin), /*possible_overwrite=*/false);

    return outpoint;
};
