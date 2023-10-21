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

#include <crypto/poly1305.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_poly1305)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, POLY1305_KEYLEN);
    const std::vector<uint8_t> in = ConsumeRandomLengthByteVector(fuzzed_data_provider);

    std::vector<uint8_t> tag_out(POLY1305_TAGLEN);
    poly1305_auth(tag_out.data(), in.data(), in.size(), key.data());
}
