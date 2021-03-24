// Copyright (c) 2016 The Bitcoin Core developers
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

#include "policy/rbf.h"

bool SignalsOptInRBF(const CTransaction &tx)
{
    BOOST_FOREACH(const CTxIn &txin, tx.vin) {
        if (txin.nSequence < std::numeric_limits<unsigned int>::max()-1) {
            return true;
        }
    }
    return false;
}

RBFTransactionState IsRBFOptIn(const CTransaction &tx, CTxMemPool &pool)
{
    AssertLockHeld(pool.cs);

    CTxMemPool::setEntries setAncestors;

    // First check the transaction itself.
    if (SignalsOptInRBF(tx)) {
        return RBF_TRANSACTIONSTATE_REPLACEABLE_BIP125;
    }

    // If this transaction is not in our mempool, then we can't be sure
    // we will know about all its inputs.
    if (!pool.exists(tx.GetHash())) {
        return RBF_TRANSACTIONSTATE_UNKNOWN;
    }

    // If all the inputs have nSequence >= maxint-1, it still might be
    // signaled for RBF if any unconfirmed parents have signaled.
    uint64_t noLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    CTxMemPoolEntry entry = *pool.mapTx.find(tx.GetHash());
    pool.CalculateMemPoolAncestors(entry, setAncestors, noLimit, noLimit, noLimit, noLimit, dummy, false);

    BOOST_FOREACH(CTxMemPool::txiter it, setAncestors) {
        if (SignalsOptInRBF(it->GetTx())) {
            return RBF_TRANSACTIONSTATE_REPLACEABLE_BIP125;
        }
    }
    return RBF_TRANSACTIONSTATE_FINAL;
}
