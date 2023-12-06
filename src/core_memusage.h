// Copyright (c) 2015-2018 The Bitcoin Core developers
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

#ifndef FREICOIN_CORE_MEMUSAGE_H
#define FREICOIN_CORE_MEMUSAGE_H

#include <primitives/transaction.h>
#include <primitives/block.h>
#include <memusage.h>

static inline size_t RecursiveDynamicUsage(const CScript& script) {
    return memusage::DynamicUsage(script);
}

static inline size_t RecursiveDynamicUsage(const COutPoint& out) {
    return 0;
}

static inline size_t RecursiveDynamicUsage(const CTxIn& in) {
    size_t mem = RecursiveDynamicUsage(in.scriptSig) + RecursiveDynamicUsage(in.prevout) + memusage::DynamicUsage(in.scriptWitness.stack);
    for (std::vector<std::vector<unsigned char> >::const_iterator it = in.scriptWitness.stack.begin(); it != in.scriptWitness.stack.end(); it++) {
         mem += memusage::DynamicUsage(*it);
    }
    return mem;
}

static inline size_t RecursiveDynamicUsage(const CTxOut& out) {
    return RecursiveDynamicUsage(out.scriptPubKey);
}

static inline size_t RecursiveDynamicUsage(const CTransaction& tx) {
    size_t mem = memusage::DynamicUsage(tx.vin) + memusage::DynamicUsage(tx.vout);
    for (std::vector<CTxIn>::const_iterator it = tx.vin.begin(); it != tx.vin.end(); it++) {
        mem += RecursiveDynamicUsage(*it);
    }
    for (std::vector<CTxOut>::const_iterator it = tx.vout.begin(); it != tx.vout.end(); it++) {
        mem += RecursiveDynamicUsage(*it);
    }
    return mem;
}

static inline size_t RecursiveDynamicUsage(const CMutableTransaction& tx) {
    size_t mem = memusage::DynamicUsage(tx.vin) + memusage::DynamicUsage(tx.vout);
    for (std::vector<CTxIn>::const_iterator it = tx.vin.begin(); it != tx.vin.end(); it++) {
        mem += RecursiveDynamicUsage(*it);
    }
    for (std::vector<CTxOut>::const_iterator it = tx.vout.begin(); it != tx.vout.end(); it++) {
        mem += RecursiveDynamicUsage(*it);
    }
    return mem;
}

static inline size_t RecursiveDynamicUsage(const CBlock& block) {
    size_t mem = memusage::DynamicUsage(block.vtx);
    for (const auto& tx : block.vtx) {
        mem += memusage::DynamicUsage(tx) + RecursiveDynamicUsage(*tx);
    }
    return mem;
}

static inline size_t RecursiveDynamicUsage(const CBlockLocator& locator) {
    return memusage::DynamicUsage(locator.vHave);
}

template<typename X>
static inline size_t RecursiveDynamicUsage(const std::shared_ptr<X>& p) {
    return p ? memusage::DynamicUsage(p) + RecursiveDynamicUsage(*p) : 0;
}

#endif // FREICOIN_CORE_MEMUSAGE_H
