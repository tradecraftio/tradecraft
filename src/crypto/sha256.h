// Copyright (c) 2014-2016 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#ifndef FREICOIN_CRYPTO_SHA256_H
#define FREICOIN_CRYPTO_SHA256_H

#include <stdint.h>
#include <stdlib.h>

/** A hasher class for SHA-256. */
class CSHA256
{
private:
    uint32_t s[8];
    unsigned char buf[64];
    uint64_t bytes;

public:
    static const size_t OUTPUT_SIZE = 32;

    CSHA256();
    CSHA256(const unsigned char iv[OUTPUT_SIZE]);
    CSHA256(const unsigned char hash[OUTPUT_SIZE], const unsigned char* buffer, uint64_t length);
    CSHA256& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    void Midstate(unsigned char hash[OUTPUT_SIZE], unsigned char* buffer, uint64_t* length);
    CSHA256& Reset();
};

#endif // FREICOIN_CRYPTO_SHA256_H
