// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
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

#ifndef FREICOIN_POLICY_FEERATE_H
#define FREICOIN_POLICY_FEERATE_H

#include <amount.h>
#include <serialize.h>

#include <string>

extern const std::string CURRENCY_UNIT;

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
    template<typename I>
    CFeeRate(const I _nKriaPerK): nKriaPerK(_nKriaPerK) {
        // We've previously had bugs creep in from silent double->int conversion...
        static_assert(std::is_integral<I>::value, "CFeeRate should be used without floats");
    }
    /** Constructor for a fee rate in kria per kB. The size in bytes must not exceed (2^63 - 1)*/
    CFeeRate(const CAmount& nFeePaid, size_t nBytes);
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
    friend bool operator!=(const CFeeRate& a, const CFeeRate& b) { return a.nKriaPerK != b.nKriaPerK; }
    CFeeRate& operator+=(const CFeeRate& a) { nKriaPerK += a.nKriaPerK; return *this; }
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nKriaPerK);
    }
};

#endif //  FREICOIN_POLICY_FEERATE_H
