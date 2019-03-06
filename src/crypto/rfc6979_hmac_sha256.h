// Copyright (c) 2014 The Bitcoin developers
// Copyright (c) 2011-2019 The Freicoin developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#ifndef FREICOIN_RFC6979_HMAC_SHA256_H
#define FREICOIN_RFC6979_HMAC_SHA256_H

#include "crypto/hmac_sha256.h"

#include <stdint.h>
#include <stdlib.h>

/** The RFC 6979 PRNG using HMAC-SHA256. */
class RFC6979_HMAC_SHA256
{
private:
    unsigned char V[CHMAC_SHA256::OUTPUT_SIZE];
    unsigned char K[CHMAC_SHA256::OUTPUT_SIZE];
    bool retry;

public:
    /**
     * Construct a new RFC6979 PRNG, using the given key and message.
     * The message is assumed to be already hashed.
     */
    RFC6979_HMAC_SHA256(const unsigned char* key, size_t keylen, const unsigned char* msg, size_t msglen);

    /**
     * Generate a byte array.
     */
    void Generate(unsigned char* output, size_t outputlen);

    ~RFC6979_HMAC_SHA256();
};

#endif // FREICOIN_RFC6979_HMAC_SHA256_H
