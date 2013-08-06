// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
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

#ifndef FREICOIN_AMOUNT_H
#define FREICOIN_AMOUNT_H

#include "serialize.h"

#include <stdlib.h>
#include <string>

typedef int64_t CAmount;

static const CAmount COIN = 100000000;
static const CAmount CENT = 1000000;

/** No amount larger than this (in kria) is valid */
static const CAmount MAX_MONEY = 9007199254740991LL; // 2^53 - 1
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
/** Adjust to present value by subtracting (or adding) demurrage */
CAmount TimeAdjustValueForward(const CAmount& initial_value, uint32_t distance);
CAmount TimeAdjustValueReverse(const CAmount& initial_value, uint32_t distance);
CAmount GetTimeAdjustedValue(const CAmount& initial_value, int relative_depth);

/** Type-safe wrapper class to for fee rates
 * (how much to pay based on transaction size)
 */
class CFeeRate
{
private:
    CAmount nKriaPerK; // unit is kria-per-1,000-bytes
public:
    CFeeRate() : nKriaPerK(0) { }
    explicit CFeeRate(const CAmount& _nKriaPerK): nKriaPerK(_nKriaPerK) { }
    CFeeRate(const CAmount& nFeePaid, size_t nSize);
    CFeeRate(const CFeeRate& other) { nKriaPerK = other.nKriaPerK; }

    CAmount GetFee(size_t size) const; // unit returned is kria
    CAmount GetFeePerK() const { return GetFee(1000); } // kria-per-1000-bytes

    friend bool operator<(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK < b.nKriaPerK; }
    friend bool operator>(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK > b.nKriaPerK; }
    friend bool operator==(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK == b.nKriaPerK; }
    friend bool operator<=(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK <= b.nKriaPerK; }
    friend bool operator>=(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK >= b.nKriaPerK; }
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nKriaPerK);
    }
};

#endif //  FREICOIN_AMOUNT_H
