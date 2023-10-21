// Copyright (c) 2019-2020 The Bitcoin Core developers
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

#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <version.h>

#include <cassert>

FUZZ_TARGET(tx_in)
{
    DataStream ds{buffer};
    CTxIn tx_in;
    try {
        ds >> tx_in;
    } catch (const std::ios_base::failure&) {
        return;
    }

    (void)GetTransactionInputWeight(tx_in);
    (void)GetVirtualTransactionInputSize(tx_in);
    (void)RecursiveDynamicUsage(tx_in);

    (void)tx_in.ToString();
}
