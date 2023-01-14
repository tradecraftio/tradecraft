// Copyright (c) 2010-2021 The Bitcoin Core developers
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

#include <util/error.h>

#include <tinyformat.h>
#include <util/translation.h>

#include <cassert>
#include <string>

bilingual_str TransactionErrorString(const TransactionError err)
{
    switch (err) {
        case TransactionError::OK:
            return Untranslated("No error");
        case TransactionError::MISSING_INPUTS:
            return Untranslated("Inputs missing or spent");
        case TransactionError::ALREADY_IN_CHAIN:
            return Untranslated("Transaction already in block chain");
        case TransactionError::P2P_DISABLED:
            return Untranslated("Peer-to-peer functionality missing or disabled");
        case TransactionError::MEMPOOL_REJECTED:
            return Untranslated("Transaction rejected by mempool");
        case TransactionError::MEMPOOL_ERROR:
            return Untranslated("Mempool internal error");
        case TransactionError::INVALID_PSBT:
            return Untranslated("PSBT is not well-formed");
        case TransactionError::PSBT_MISMATCH:
            return Untranslated("PSBTs not compatible (different transactions)");
        case TransactionError::SIGHASH_MISMATCH:
            return Untranslated("Specified sighash value does not match value stored in PSBT");
        case TransactionError::MAX_FEE_EXCEEDED:
            return Untranslated("Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)");
        case TransactionError::EXTERNAL_SIGNER_NOT_FOUND:
            return Untranslated("External signer not found");
        case TransactionError::EXTERNAL_SIGNER_FAILED:
            return Untranslated("External signer failed to sign");
        case TransactionError::INVALID_PACKAGE:
            return Untranslated("Transaction rejected due to invalid package");
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

bilingual_str ResolveErrMsg(const std::string& optname, const std::string& strBind)
{
    return strprintf(_("Cannot resolve -%s address: '%s'"), optname, strBind);
}

bilingual_str AmountHighWarn(const std::string& optname)
{
    return strprintf(_("%s is set very high!"), optname);
}

bilingual_str AmountErrMsg(const std::string& optname, const std::string& strValue)
{
    return strprintf(_("Invalid amount for -%s=<amount>: '%s'"), optname, strValue);
}
