// Copyright (c) 2020 The Bitcoin Core developers
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

#include <key.h>
#include <secp256k1.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

bool SigHasLowR(const secp256k1_ecdsa_signature* sig);
int ecdsa_signature_parse_der_lax(const secp256k1_context* ctx, secp256k1_ecdsa_signature* sig, const unsigned char* input, size_t inputlen);

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> signature_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    if (signature_bytes.data() == nullptr) {
        return;
    }
    secp256k1_context* secp256k1_context_verify = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    secp256k1_ecdsa_signature sig_der_lax;
    const bool parsed_der_lax = ecdsa_signature_parse_der_lax(secp256k1_context_verify, &sig_der_lax, signature_bytes.data(), signature_bytes.size()) == 1;
    if (parsed_der_lax) {
        ECC_Start();
        (void)SigHasLowR(&sig_der_lax);
        ECC_Stop();
    }
    secp256k1_context_destroy(secp256k1_context_verify);
}
