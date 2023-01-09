// Copyright (c) 2010-2020 The Bitcoin Core developers
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

#ifndef FREICOIN_UTIL_ERROR_H
#define FREICOIN_UTIL_ERROR_H

/**
 * util/error.h is a common place for definitions of simple error types and
 * string functions. Types and functions defined here should not require any
 * outside dependencies.
 *
 * Error types defined here can be used in different parts of the
 * codebase, to avoid the need to write boilerplate code catching and
 * translating errors passed across wallet/node/rpc/gui code boundaries.
 */

#include <string>

struct bilingual_str;

enum class TransactionError {
    OK, //!< No error
    MISSING_INPUTS,
    ALREADY_IN_CHAIN,
    P2P_DISABLED,
    MEMPOOL_REJECTED,
    MEMPOOL_ERROR,
    INVALID_PST,
    PST_MISMATCH,
    SIGHASH_MISMATCH,
    MAX_FEE_EXCEEDED,
    EXTERNAL_SIGNER_NOT_FOUND,
    EXTERNAL_SIGNER_FAILED,
};

bilingual_str TransactionErrorString(const TransactionError error);

bilingual_str ResolveErrMsg(const std::string& optname, const std::string& strBind);

bilingual_str AmountHighWarn(const std::string& optname);

bilingual_str AmountErrMsg(const std::string& optname, const std::string& strValue);

#endif // FREICOIN_UTIL_ERROR_H
