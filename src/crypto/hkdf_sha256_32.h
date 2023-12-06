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

#ifndef FREICOIN_CRYPTO_HKDF_SHA256_32_H
#define FREICOIN_CRYPTO_HKDF_SHA256_32_H

#include <crypto/hmac_sha256.h>

#include <cstdlib>
#include <stdint.h>

/** A rfc5869 HKDF implementation with HMAC_SHA256 and fixed key output length of 32 bytes (L=32) */
class CHKDF_HMAC_SHA256_L32
{
private:
    unsigned char m_prk[32];
    static const size_t OUTPUT_SIZE = 32;

public:
    CHKDF_HMAC_SHA256_L32(const unsigned char* ikm, size_t ikmlen, const std::string& salt);
    void Expand32(const std::string& info, unsigned char hash[OUTPUT_SIZE]);
};

#endif // FREICOIN_CRYPTO_HKDF_SHA256_32_H
