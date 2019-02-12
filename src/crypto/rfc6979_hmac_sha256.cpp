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

#include "crypto/rfc6979_hmac_sha256.h"

#include <string.h>

#include <algorithm>

static const unsigned char zero[1] = {0x00};
static const unsigned char one[1] = {0x01};

RFC6979_HMAC_SHA256::RFC6979_HMAC_SHA256(const unsigned char* key, size_t keylen, const unsigned char* msg, size_t msglen) : retry(false)
{
    memset(V, 0x01, sizeof(V));
    memset(K, 0x00, sizeof(K));

    CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Write(zero, sizeof(zero)).Write(key, keylen).Write(msg, msglen).Finalize(K);
    CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Finalize(V);
    CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Write(one, sizeof(one)).Write(key, keylen).Write(msg, msglen).Finalize(K);
    CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Finalize(V);
}

RFC6979_HMAC_SHA256::~RFC6979_HMAC_SHA256()
{
    memset(V, 0x01, sizeof(V));
    memset(K, 0x00, sizeof(K));
}

void RFC6979_HMAC_SHA256::Generate(unsigned char* output, size_t outputlen)
{
    if (retry) {
        CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Write(zero, sizeof(zero)).Finalize(K);
        CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Finalize(V);
    }

    while (outputlen > 0) {
        CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Finalize(V);
        size_t len = std::min(outputlen, sizeof(V));
        memcpy(output, V, len);
        output += len;
        outputlen -= len;
    }

    retry = true;
}
