// Copyright (c) 2018-2022 The Bitcoin Core developers
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

#ifndef FREICOIN_UTIL_GOLOMBRICE_H
#define FREICOIN_UTIL_GOLOMBRICE_H

#include <util/fastrange.h>

#include <streams.h>

#include <cstdint>

template <typename OStream>
void GolombRiceEncode(BitStreamWriter<OStream>& bitwriter, uint8_t P, uint64_t x)
{
    // Write quotient as unary-encoded: q 1's followed by one 0.
    uint64_t q = x >> P;
    while (q > 0) {
        int nbits = q <= 64 ? static_cast<int>(q) : 64;
        bitwriter.Write(~0ULL, nbits);
        q -= nbits;
    }
    bitwriter.Write(0, 1);

    // Write the remainder in P bits. Since the remainder is just the bottom
    // P bits of x, there is no need to mask first.
    bitwriter.Write(x, P);
}

template <typename IStream>
uint64_t GolombRiceDecode(BitStreamReader<IStream>& bitreader, uint8_t P)
{
    // Read unary-encoded quotient: q 1's followed by one 0.
    uint64_t q = 0;
    while (bitreader.Read(1) == 1) {
        ++q;
    }

    uint64_t r = bitreader.Read(P);

    return (q << P) + r;
}

#endif // FREICOIN_UTIL_GOLOMBRICE_H
