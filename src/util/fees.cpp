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

#include <util/fees.h>

#include <policy/fees.h>

#include <map>
#include <string>

std::string StringForFeeReason(FeeReason reason) {
    static const std::map<FeeReason, std::string> fee_reason_strings = {
        {FeeReason::NONE, "None"},
        {FeeReason::HALF_ESTIMATE, "Half Target 60% Threshold"},
        {FeeReason::FULL_ESTIMATE, "Target 85% Threshold"},
        {FeeReason::DOUBLE_ESTIMATE, "Double Target 95% Threshold"},
        {FeeReason::CONSERVATIVE, "Conservative Double Target longer horizon"},
        {FeeReason::MEMPOOL_MIN, "Mempool Min Fee"},
        {FeeReason::PAYTXFEE, "PayTxFee set"},
        {FeeReason::FALLBACK, "Fallback fee"},
        {FeeReason::REQUIRED, "Minimum Required Fee"},
    };
    auto reason_string = fee_reason_strings.find(reason);

    if (reason_string == fee_reason_strings.end()) return "Unknown";

    return reason_string->second;
}

bool FeeModeFromString(const std::string& mode_string, FeeEstimateMode& fee_estimate_mode) {
    static const std::map<std::string, FeeEstimateMode> fee_modes = {
        {"UNSET", FeeEstimateMode::UNSET},
        {"ECONOMICAL", FeeEstimateMode::ECONOMICAL},
        {"CONSERVATIVE", FeeEstimateMode::CONSERVATIVE},
    };
    auto mode = fee_modes.find(mode_string);

    if (mode == fee_modes.end()) return false;

    fee_estimate_mode = mode->second;
    return true;
}
