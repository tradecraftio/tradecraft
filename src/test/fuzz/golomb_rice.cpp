// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#include <blockfilter.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/bytevectorhash.h>
#include <util/golombrice.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iosfwd>
#include <unordered_set>
#include <vector>

namespace {

uint64_t HashToRange(const std::vector<uint8_t>& element, const uint64_t f)
{
    const uint64_t hash = CSipHasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL)
                              .Write(element)
                              .Finalize();
    return FastRange64(hash, f);
}

std::vector<uint64_t> BuildHashedSet(const std::unordered_set<std::vector<uint8_t>, ByteVectorHash>& elements, const uint64_t f)
{
    std::vector<uint64_t> hashed_elements;
    hashed_elements.reserve(elements.size());
    for (const std::vector<uint8_t>& element : elements) {
        hashed_elements.push_back(HashToRange(element, f));
    }
    std::sort(hashed_elements.begin(), hashed_elements.end());
    return hashed_elements;
}
} // namespace

FUZZ_TARGET(golomb_rice)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::vector<uint8_t> golomb_rice_data;
    std::vector<uint64_t> encoded_deltas;
    {
        std::unordered_set<std::vector<uint8_t>, ByteVectorHash> elements;
        const int n = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 512);
        for (int i = 0; i < n; ++i) {
            elements.insert(ConsumeRandomLengthByteVector(fuzzed_data_provider, 16));
        }
        VectorWriter stream{golomb_rice_data, 0};
        WriteCompactSize(stream, static_cast<uint32_t>(elements.size()));
        BitStreamWriter bitwriter{stream};
        if (!elements.empty()) {
            uint64_t last_value = 0;
            for (const uint64_t value : BuildHashedSet(elements, static_cast<uint64_t>(elements.size()) * static_cast<uint64_t>(BASIC_FILTER_M))) {
                const uint64_t delta = value - last_value;
                encoded_deltas.push_back(delta);
                GolombRiceEncode(bitwriter, BASIC_FILTER_P, delta);
                last_value = value;
            }
        }
        bitwriter.Flush();
    }

    std::vector<uint64_t> decoded_deltas;
    {
        SpanReader stream{golomb_rice_data};
        BitStreamReader bitreader{stream};
        const uint32_t n = static_cast<uint32_t>(ReadCompactSize(stream));
        for (uint32_t i = 0; i < n; ++i) {
            decoded_deltas.push_back(GolombRiceDecode(bitreader, BASIC_FILTER_P));
        }
    }

    assert(encoded_deltas == decoded_deltas);

    {
        const std::vector<uint8_t> random_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider, 1024);
        SpanReader stream{random_bytes};
        uint32_t n;
        try {
            n = static_cast<uint32_t>(ReadCompactSize(stream));
        } catch (const std::ios_base::failure&) {
            return;
        }
        BitStreamReader bitreader{stream};
        for (uint32_t i = 0; i < std::min<uint32_t>(n, 1024); ++i) {
            try {
                (void)GolombRiceDecode(bitreader, BASIC_FILTER_P);
            } catch (const std::ios_base::failure&) {
            }
        }
    }
}
