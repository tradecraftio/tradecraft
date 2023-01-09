// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <crypto/aes.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_aes256cbc)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES256_KEYSIZE);
    const std::vector<uint8_t> iv = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES_BLOCKSIZE);
    const bool pad = fuzzed_data_provider.ConsumeBool();

    AES256CBCEncrypt encrypt{key.data(), iv.data(), pad};
    AES256CBCDecrypt decrypt{key.data(), iv.data(), pad};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> plaintext = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        std::vector<uint8_t> ciphertext(plaintext.size() + AES_BLOCKSIZE);
        const int encrypt_ret = encrypt.Encrypt(plaintext.data(), plaintext.size(), ciphertext.data());
        ciphertext.resize(encrypt_ret);
        std::vector<uint8_t> decrypted_plaintext(ciphertext.size());
        const int decrypt_ret = decrypt.Decrypt(ciphertext.data(), ciphertext.size(), decrypted_plaintext.data());
        decrypted_plaintext.resize(decrypt_ret);
        assert(decrypted_plaintext == plaintext || (!pad && plaintext.size() % AES_BLOCKSIZE != 0 && encrypt_ret == 0 && decrypt_ret == 0));
    }
}
