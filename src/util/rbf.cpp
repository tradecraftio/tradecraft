// Copyright (c) 2016-2019 The Bitcoin Core developers
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

#include <util/rbf.h>

#include <primitives/transaction.h>

bool SignalsOptInRBF(const CTransaction &tx)
{
    for (const CTxIn &txin : tx.vin) {
        if (txin.nSequence <= MAX_BIP125_RBF_SEQUENCE) {
            return true;
        }
    }
    return false;
}
