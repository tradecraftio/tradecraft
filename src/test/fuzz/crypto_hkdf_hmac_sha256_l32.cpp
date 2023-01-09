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

#include <crypto/hkdf_sha256_32.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(crypto_hkdf_hmac_sha256_l32)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> initial_key_material = ConsumeRandomLengthByteVector(fuzzed_data_provider);

    CHKDF_HMAC_SHA256_L32 hkdf_hmac_sha256_l32(initial_key_material.data(), initial_key_material.size(), fuzzed_data_provider.ConsumeRandomLengthString(1024));
    while (fuzzed_data_provider.ConsumeBool()) {
        std::vector<uint8_t> out(32);
        hkdf_hmac_sha256_l32.Expand32(fuzzed_data_provider.ConsumeRandomLengthString(128), out.data());
    }
}
