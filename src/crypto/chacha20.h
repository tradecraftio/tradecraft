// Copyright (c) 2017-2019 The Bitcoin Core developers
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

#ifndef FREICOIN_CRYPTO_CHACHA20_H
#define FREICOIN_CRYPTO_CHACHA20_H

#include <stdint.h>
#include <stdlib.h>

/** A class for ChaCha20 256-bit stream cipher developed by Daniel J. Bernstein
    https://cr.yp.to/chacha/chacha-20080128.pdf */
class ChaCha20
{
private:
    uint32_t input[16];

public:
    ChaCha20();
    ChaCha20(const unsigned char* key, size_t keylen);
    void SetKey(const unsigned char* key, size_t keylen); //!< set key with flexible keylength; 256bit recommended */
    void SetIV(uint64_t iv); // set the 64bit nonce
    void Seek(uint64_t pos); // set the 64bit block counter

    /** outputs the keystream of size <bytes> into <c> */
    void Keystream(unsigned char* c, size_t bytes);

    /** enciphers the message <input> of length <bytes> and write the enciphered representation into <output>
     *  Used for encryption and decryption (XOR)
     */
    void Crypt(const unsigned char* input, unsigned char* output, size_t bytes);
};

#endif // FREICOIN_CRYPTO_CHACHA20_H
