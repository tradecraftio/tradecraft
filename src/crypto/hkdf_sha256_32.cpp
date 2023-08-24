// Copyright (c) 2018-2019 The Bitcoin Core developers
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

#include <crypto/hkdf_sha256_32.h>

#include <assert.h>
#include <string.h>

CHKDF_HMAC_SHA256_L32::CHKDF_HMAC_SHA256_L32(const unsigned char* ikm, size_t ikmlen, const std::string& salt)
{
    CHMAC_SHA256((const unsigned char*)salt.data(), salt.size()).Write(ikm, ikmlen).Finalize(m_prk);
}

void CHKDF_HMAC_SHA256_L32::Expand32(const std::string& info, unsigned char hash[OUTPUT_SIZE])
{
    // expand a 32byte key (single round)
    assert(info.size() <= 128);
    static const unsigned char one[1] = {1};
    CHMAC_SHA256(m_prk, 32).Write((const unsigned char*)info.data(), info.size()).Write(one, 1).Finalize(hash);
}
