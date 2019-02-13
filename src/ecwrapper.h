// Copyright (c) 2009-2014 The Bitcoin developers
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

#ifndef FREICOIN_ECWRAPPER_H
#define FREICOIN_ECWRAPPER_H

#include <cstddef>
#include <vector>

#include <openssl/ec.h>

class uint256;

/** RAII Wrapper around OpenSSL's EC_KEY */
class CECKey {
private:
    EC_KEY *pkey;

public:
    CECKey();
    ~CECKey();

    void GetPubKey(std::vector<unsigned char>& pubkey, bool fCompressed);
    bool SetPubKey(const unsigned char* pubkey, size_t size);
    bool Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig);

    /**
     * reconstruct public key from a compact signature
     * This is only slightly more CPU intensive than just verifying it.
     * If this function succeeds, the recovered public key is guaranteed to be valid
     * (the signature is a valid signature of the given data for that key)
     */
    bool Recover(const uint256 &hash, const unsigned char *p64, int rec);

    bool TweakPublic(const unsigned char vchTweak[32]);
    static bool SanityCheck();
};

#endif // FREICOIN_ECWRAPPER_H
