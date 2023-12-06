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

    const auto key = ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, Poly1305::KEYLEN);
    const auto in = ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider);

    std::vector<std::byte> tag_out(Poly1305::TAGLEN);
    Poly1305{key}.Update(in).Finalize(tag_out);
}

FUZZ_TARGET(crypto_poly1305_split)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    // Read key and instantiate two Poly1305 objects with it.
    auto key = provider.ConsumeBytes<std::byte>(Poly1305::KEYLEN);
    key.resize(Poly1305::KEYLEN);
    Poly1305 poly_full{key}, poly_split{key};

    // Vector that holds all bytes processed so far.
    std::vector<std::byte> total_input;

    // Process input in pieces.
    LIMITED_WHILE(provider.remaining_bytes(), 100) {
        auto in = ConsumeRandomLengthByteVector<std::byte>(provider);
        poly_split.Update(in);
        // Update total_input to match what was processed.
        total_input.insert(total_input.end(), in.begin(), in.end());
    }

    // Process entire input at once.
    poly_full.Update(total_input);

    // Verify both agree.
    std::array<std::byte, Poly1305::TAGLEN> tag_split, tag_full;
    poly_split.Finalize(tag_split);
    poly_full.Finalize(tag_full);
    assert(tag_full == tag_split);
}
