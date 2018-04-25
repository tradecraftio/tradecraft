// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
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

#ifndef FREICOIN_AMOUNT_H
#define FREICOIN_AMOUNT_H

#include "serialize.h"

#include <stdlib.h>
#include <string>

/** Amount in kria (Can be negative) */
typedef int64_t CAmount;

static const CAmount COIN = 100000000;
static const CAmount CENT = 1000000;

extern const std::string CURRENCY_UNIT;

/** No amount larger than this (in kria) is valid.
 *
 * Note that this constant is *not* the total money supply, which in Freicoin
 * currently happens to be less than 21,000,000 FRC for various reasons, but
 * rather a sanity check. As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 * */
static const CAmount MAX_MONEY = 21000000 * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
/** Adjust to present value by subtracting (or adding) demurrage */
static const bool DEFAULT_DISABLE_TIME_ADJUST = false;
extern bool disable_time_adjust;
CAmount TimeAdjustValueForward(const CAmount& initial_value, uint32_t distance);
CAmount TimeAdjustValueReverse(const CAmount& initial_value, uint32_t distance);
CAmount GetTimeAdjustedValue(const CAmount& initial_value, int relative_depth);

/**
 * Fee rate in kria per kilobyte: CAmount / kB
 */
class CFeeRate
{
private:
    CAmount nKriaPerK; // unit is kria-per-1,000-bytes
public:
    /** Fee rate of 0 kria per kB */
    CFeeRate() : nKriaPerK(0) { }
    explicit CFeeRate(const CAmount& _nKriaPerK): nKriaPerK(_nKriaPerK) { }
    /** Constructor for a fee rate in kria per kB. The size in bytes must not exceed (2^63 - 1)*/
    CFeeRate(const CAmount& nFeePaid, size_t nBytes);
    CFeeRate(const CFeeRate& other) { nKriaPerK = other.nKriaPerK; }
    /**
     * Return the fee in kria for the given size in bytes.
     */
    CAmount GetFee(size_t nBytes) const;
    /**
     * Return the fee in kria for a size of 1000 bytes
     */
    CAmount GetFeePerK() const { return GetFee(1000); }
    friend bool operator<(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK < b.nKriaPerK; }
    friend bool operator>(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK > b.nKriaPerK; }
    friend bool operator==(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK == b.nKriaPerK; }
    friend bool operator<=(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK <= b.nKriaPerK; }
    friend bool operator>=(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK >= b.nKriaPerK; }
    CFeeRate& operator+=(const CFeeRate& a) { nKriaPerK += a.nKriaPerK; return *this; }
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nKriaPerK);
    }
};

#endif //  FREICOIN_AMOUNT_H
