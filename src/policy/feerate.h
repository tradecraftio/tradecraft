// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
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

#ifndef FREICOIN_POLICY_FEERATE_H
#define FREICOIN_POLICY_FEERATE_H

#include <amount.h>
#include <serialize.h>

#include <string>

const std::string CURRENCY_UNIT = "FRC"; // One formatted unit
const std::string CURRENCY_ATOM = "sat"; // One indivisible minimum value unit

/* Used to determine type of fee estimation requested */
enum class FeeEstimateMode {
    UNSET,        //!< Use default settings based on other criteria
    ECONOMICAL,   //!< Force estimateSmartFee to use non-conservative estimates
    CONSERVATIVE, //!< Force estimateSmartFee to use conservative estimates
    FRC_KVB,      //!< Use FRC/kvB fee rate unit
    SAT_VB,       //!< Use sat/vB fee rate unit
};

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
    explicit CFeeRate(const I _nKriaPerK): nKriaPerK(_nKriaPerK) {
        // We've previously had bugs creep in from silent double->int conversion...
        static_assert(std::is_integral<I>::value, "CFeeRate should be used without floats");
    }
    /** Constructor for a fee rate in kria per kvB (sat/kvB).
     *
     *  Passing a num_bytes value of COIN (1e8) returns a fee rate in kria per vB (sat/vB),
     *  e.g. (nFeePaid * 1e8 / 1e3) == (nFeePaid / 1e5),
     *  where 1e5 is the ratio to convert from FRC/kvB to sat/vB.
     */
    CFeeRate(const CAmount& nFeePaid, uint32_t num_bytes);
    /**
     * Return the fee in kria for the given size in bytes.
     * If the calculated fee would have fractional kria, then the returned fee will always be rounded up to the nearest kria.
     */
    CAmount GetFee(uint32_t num_bytes) const;
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
    std::string ToString(const FeeEstimateMode& fee_estimate_mode = FeeEstimateMode::FRC_KVB) const;

    SERIALIZE_METHODS(CFeeRate, obj) { READWRITE(obj.nKriaPerK); }
};

#endif //  FREICOIN_POLICY_FEERATE_H
