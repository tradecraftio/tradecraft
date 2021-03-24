// Copyright (c) 2014 The Bitcoin Core developers
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

#ifndef BITCOIN_CRYPTO_COMMON_H
#define BITCOIN_CRYPTO_COMMON_H

#if defined(HAVE_CONFIG_H)
#include "bitcoin-config.h"
#endif

#include <stdint.h>
#include <string.h>

#include "compat/endian.h"

uint16_t static inline ReadLE16(const unsigned char* ptr)
{
    uint16_t x;
    memcpy((char*)&x, ptr, 2);
    return le16toh(x);
}

uint32_t static inline ReadLE32(const unsigned char* ptr)
{
    uint32_t x;
    memcpy((char*)&x, ptr, 4);
    return le32toh(x);
}

uint64_t static inline ReadLE64(const unsigned char* ptr)
{
    uint64_t x;
    memcpy((char*)&x, ptr, 8);
    return le64toh(x);
}

void static inline WriteLE16(unsigned char* ptr, uint16_t x)
{
    uint16_t v = htole16(x);
    memcpy(ptr, (char*)&v, 2);
}

void static inline WriteLE32(unsigned char* ptr, uint32_t x)
{
    uint32_t v = htole32(x);
    memcpy(ptr, (char*)&v, 4);
}

void static inline WriteLE64(unsigned char* ptr, uint64_t x)
{
    uint64_t v = htole64(x);
    memcpy(ptr, (char*)&v, 8);
}

uint32_t static inline ReadBE32(const unsigned char* ptr)
{
    uint32_t x;
    memcpy((char*)&x, ptr, 4);
    return be32toh(x);
}

uint64_t static inline ReadBE64(const unsigned char* ptr)
{
    uint64_t x;
    memcpy((char*)&x, ptr, 8);
    return be64toh(x);
}

void static inline WriteBE32(unsigned char* ptr, uint32_t x)
{
    uint32_t v = htobe32(x);
    memcpy(ptr, (char*)&v, 4);
}

void static inline WriteBE64(unsigned char* ptr, uint64_t x)
{
    uint64_t v = htobe64(x);
    memcpy(ptr, (char*)&v, 8);
}

#endif // BITCOIN_CRYPTO_COMMON_H
