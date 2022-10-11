// Copyright (c) 2010-2019 The Bitcoin Core developers
// Copyright (c) 2011-2022 The Freicoin Developers
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
#include <util/system.h>
#include <util/translation.h>

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
            return Untranslated("Transaction rejected by AcceptToMemoryPool");
        case TransactionError::MEMPOOL_ERROR:
            return Untranslated("AcceptToMemoryPool failed");
        case TransactionError::INVALID_PST:
            return Untranslated("PST is not well-formed");
        case TransactionError::PST_MISMATCH:
            return Untranslated("PSTs not compatible (different transactions)");
        case TransactionError::SIGHASH_MISMATCH:
            return Untranslated("Specified sighash value does not match value stored in PST");
        case TransactionError::MAX_FEE_EXCEEDED:
            return Untranslated("Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)");
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
