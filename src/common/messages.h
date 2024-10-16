// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

//! @file common/messages.h is a home for simple string functions returning
//! descriptive messages that are used in RPC and GUI interfaces or log
//! messages, and are called in different parts of the codebase across
//! node/wallet/gui boundaries.

#ifndef FREICOIN_COMMON_MESSAGES_H
#define FREICOIN_COMMON_MESSAGES_H

#include <string>

struct bilingual_str;

enum class FeeEstimateMode;
enum class FeeReason;
namespace node {
enum class TransactionError;
} // namespace node

namespace common {
enum class PSTError;
bool FeeModeFromString(const std::string& mode_string, FeeEstimateMode& fee_estimate_mode);
std::string StringForFeeReason(FeeReason reason);
std::string FeeModes(const std::string& delimiter);
std::string FeeModeInfo(std::pair<std::string, FeeEstimateMode>& mode);
std::string FeeModesDetail(std::string default_info);
std::string InvalidEstimateModeErrorMessage();
bilingual_str PSTErrorString(PSTError error);
bilingual_str TransactionErrorString(const node::TransactionError error);
bilingual_str ResolveErrMsg(const std::string& optname, const std::string& strBind);
bilingual_str InvalidPortErrMsg(const std::string& optname, const std::string& strPort);
bilingual_str AmountHighWarn(const std::string& optname);
bilingual_str AmountErrMsg(const std::string& optname, const std::string& strValue);
} // namespace common

#endif // FREICOIN_COMMON_MESSAGES_H
