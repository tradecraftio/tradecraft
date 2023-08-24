// Copyright (c) 2020 The Bitcoin Core developers
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
#include <key_io.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/message.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

void initialize_message()
{
    static const ECCVerifyHandle ecc_verify_handle;
    ECC_Start();
    SelectParams(CBaseChainParams::REGTEST);
}

FUZZ_TARGET_INIT(message, initialize_message)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string random_message = fuzzed_data_provider.ConsumeRandomLengthString(1024);
    {
        const std::vector<uint8_t> random_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        CKey private_key;
        private_key.Set(random_bytes.begin(), random_bytes.end(), fuzzed_data_provider.ConsumeBool());
        std::string signature;
        const bool message_signed = MessageSign(private_key, random_message, signature);
        if (private_key.IsValid()) {
            assert(message_signed);
            const MessageVerificationResult verification_result = MessageVerify(EncodeDestination(PKHash(private_key.GetPubKey().GetID())), signature, random_message);
            assert(verification_result == MessageVerificationResult::OK);
        }
    }
    {
        (void)MessageHash(random_message);
        (void)MessageVerify(fuzzed_data_provider.ConsumeRandomLengthString(1024), fuzzed_data_provider.ConsumeRandomLengthString(1024), random_message);
        (void)SigningResultString(fuzzed_data_provider.PickValueInArray({SigningResult::OK, SigningResult::PRIVATE_KEY_NOT_AVAILABLE, SigningResult::SIGNING_FAILED}));
    }
}
