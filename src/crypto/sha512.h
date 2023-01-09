// Copyright (c) 2014-2019 The Bitcoin Core developers
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

#ifndef FREICOIN_CRYPTO_SHA512_H
#define FREICOIN_CRYPTO_SHA512_H

#include <stdint.h>
#include <stdlib.h>

/** A hasher class for SHA-512. */
class CSHA512
{
private:
    uint64_t s[8];
    unsigned char buf[128];
    uint64_t bytes;

public:
    static constexpr size_t OUTPUT_SIZE = 64;

    CSHA512();
    CSHA512(const unsigned char iv[OUTPUT_SIZE]);
    CSHA512& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    void Midstate(unsigned char hash[OUTPUT_SIZE], unsigned char* buffer, size_t* length);
    CSHA512& Reset();
    uint64_t Size() const { return bytes; }
};

#endif // FREICOIN_CRYPTO_SHA512_H
