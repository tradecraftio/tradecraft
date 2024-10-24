// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#ifndef FREICOIN_SCRIPT_SIGCACHE_H
#define FREICOIN_SCRIPT_SIGCACHE_H

#include <script/interpreter.h>
#include <span.h>
#include <util/hasher.h>

#include <optional>
#include <vector>

// DoS prevention: limit cache size to 32MiB (over 1000000 entries on 64-bit
// systems). Due to how we count cache size, actual memory usage is slightly
// more (~32.25 MiB)
static constexpr size_t DEFAULT_MAX_SIG_CACHE_BYTES{32 << 20};

class CPubKey;

class CachingTransactionSignatureChecker : public TransactionSignatureChecker
{
private:
    bool store;

public:
    CachingTransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int64_t refheightIn, bool storeIn, PrecomputedTransactionData& txdataIn, TxSigCheckOpt opts = TxSigCheckOpt::NONE) : TransactionSignatureChecker(txToIn, nInIn, amountIn, refheightIn, txdataIn, MissingDataBehavior::ASSERT_FAIL, opts), store(storeIn) {}

    bool VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const override;
    bool VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const override;
};

[[nodiscard]] bool InitSignatureCache(size_t max_size_bytes);

#endif // FREICOIN_SCRIPT_SIGCACHE_H
