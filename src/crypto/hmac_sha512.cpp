// Copyright (c) 2014-2018 The Bitcoin Core developers
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

#include <crypto/hmac_sha512.h>

#include <string.h>

CHMAC_SHA512::CHMAC_SHA512(const unsigned char* key, size_t keylen)
{
    unsigned char rkey[128];
    if (keylen <= 128) {
        memcpy(rkey, key, keylen);
        memset(rkey + keylen, 0, 128 - keylen);
    } else {
        CSHA512().Write(key, keylen).Finalize(rkey);
        memset(rkey + 64, 0, 64);
    }

    for (int n = 0; n < 128; n++)
        rkey[n] ^= 0x5c;
    outer.Write(rkey, 128);

    for (int n = 0; n < 128; n++)
        rkey[n] ^= 0x5c ^ 0x36;
    inner.Write(rkey, 128);
}

void CHMAC_SHA512::Midstate(unsigned char hash[OUTPUT_SIZE*2], unsigned char* buffer, size_t* length)
{
    outer.Midstate(hash, NULL, NULL);
    inner.Midstate(hash+OUTPUT_SIZE, buffer, length);
}

void CHMAC_SHA512::Finalize(unsigned char hash[OUTPUT_SIZE])
{
    unsigned char temp[64];
    inner.Finalize(temp);
    outer.Write(temp, 64).Finalize(hash);
}
