// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#include <chainparams.h>
#include <chainparamsbase.h>
#include <key.h>
#include <pubkey.h>
#include <script/sigcache.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;
} // namespace

void initialize_script_sigcache()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET_INIT(script_sigcache, initialize_script_sigcache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const std::optional<CMutableTransaction> mutable_transaction = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
    const CTransaction tx{mutable_transaction ? *mutable_transaction : CMutableTransaction{}};
    const unsigned int n_in = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const CAmount amount = ConsumeMoney(fuzzed_data_provider);
    const uint32_t refheight = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    const bool store = fuzzed_data_provider.ConsumeBool();
    PrecomputedTransactionData tx_data;
    CachingTransactionSignatureChecker caching_transaction_signature_checker{mutable_transaction ? &tx : nullptr, n_in, amount, refheight, store, tx_data};
    if (fuzzed_data_provider.ConsumeBool()) {
        const auto random_bytes = fuzzed_data_provider.ConsumeBytes<unsigned char>(64);
        const XOnlyPubKey pub_key(ConsumeUInt256(fuzzed_data_provider));
        if (random_bytes.size() == 64) {
            (void)caching_transaction_signature_checker.VerifySchnorrSignature(random_bytes, pub_key, ConsumeUInt256(fuzzed_data_provider));
        }
    } else {
        const auto random_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        const auto pub_key = ConsumeDeserializable<CPubKey>(fuzzed_data_provider);
        if (pub_key) {
            if (!random_bytes.empty()) {
                (void)caching_transaction_signature_checker.VerifyECDSASignature(random_bytes, *pub_key, ConsumeUInt256(fuzzed_data_provider));
            }
        }
    }
}
