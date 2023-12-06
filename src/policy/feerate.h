// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#include <consensus/amount.h>
#include <serialize.h>


#include <cstdint>
#include <string>
#include <type_traits>

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
 * Fee rate in kria per kilovirtualbyte: CAmount / kvB
 */
class CFeeRate
{
private:
    /** Fee rate in sat/kvB (kria per 1000 virtualbytes) */
    CAmount nKriaPerK;

public:
    /** Fee rate of 0 kria per kvB */
    CFeeRate() : nKriaPerK(0) { }
    template<typename I>
    explicit CFeeRate(const I _nKriaPerK): nKriaPerK(_nKriaPerK) {
        // We've previously had bugs creep in from silent double->int conversion...
        static_assert(std::is_integral<I>::value, "CFeeRate should be used without floats");
    }

    /**
     * Construct a fee rate from a fee in kria and a vsize in vB.
     *
     * param@[in]   nFeePaid    The fee paid by a transaction, in kria
     * param@[in]   num_bytes   The vsize of a transaction, in vbytes
     */
    CFeeRate(const CAmount& nFeePaid, uint32_t num_bytes);

    /**
     * Return the fee in kria for the given vsize in vbytes.
     * If the calculated fee would have fractional kria, then the
     * returned fee will always be rounded up to the nearest kria.
     */
    CAmount GetFee(uint32_t num_bytes) const;

    /**
     * Return the fee in kria for a vsize of 1000 vbytes
     */
    CAmount GetFeePerK() const { return nKriaPerK; }
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

#endif // FREICOIN_POLICY_FEERATE_H
