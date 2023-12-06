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

#include <key.h>
#include <secp256k1.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

bool SigHasLowR(const secp256k1_ecdsa_signature* sig);
int ecdsa_signature_parse_der_lax(secp256k1_ecdsa_signature* sig, const unsigned char* input, size_t inputlen);

FUZZ_TARGET(secp256k1_ecdsa_signature_parse_der_lax)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> signature_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    if (signature_bytes.data() == nullptr) {
        return;
    }
    secp256k1_ecdsa_signature sig_der_lax;
    const bool parsed_der_lax = ecdsa_signature_parse_der_lax(&sig_der_lax, signature_bytes.data(), signature_bytes.size()) == 1;
    if (parsed_der_lax) {
        ECC_Start();
        (void)SigHasLowR(&sig_der_lax);
        ECC_Stop();
    }
}
