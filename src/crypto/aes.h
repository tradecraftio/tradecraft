// Copyright (c) 2015-2016 The Bitcoin Core developers
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
//
// C++ wrapper around ctaes, a constant-time AES implementation

#ifndef BITCOIN_CRYPTO_AES_H
#define BITCOIN_CRYPTO_AES_H

extern "C" {
#include "crypto/ctaes/ctaes.h"
}

static const int AES_BLOCKSIZE = 16;
static const int AES128_KEYSIZE = 16;
static const int AES256_KEYSIZE = 32;

/** An encryption class for AES-128. */
class AES128Encrypt
{
private:
    AES128_ctx ctx;

public:
    AES128Encrypt(const unsigned char key[16]);
    ~AES128Encrypt();
    void Encrypt(unsigned char ciphertext[16], const unsigned char plaintext[16]) const;
};

/** A decryption class for AES-128. */
class AES128Decrypt
{
private:
    AES128_ctx ctx;

public:
    AES128Decrypt(const unsigned char key[16]);
    ~AES128Decrypt();
    void Decrypt(unsigned char plaintext[16], const unsigned char ciphertext[16]) const;
};

/** An encryption class for AES-256. */
class AES256Encrypt
{
private:
    AES256_ctx ctx;

public:
    AES256Encrypt(const unsigned char key[32]);
    ~AES256Encrypt();
    void Encrypt(unsigned char ciphertext[16], const unsigned char plaintext[16]) const;
};

/** A decryption class for AES-256. */
class AES256Decrypt
{
private:
    AES256_ctx ctx;

public:
    AES256Decrypt(const unsigned char key[32]);
    ~AES256Decrypt();
    void Decrypt(unsigned char plaintext[16], const unsigned char ciphertext[16]) const;
};

class AES256CBCEncrypt
{
public:
    AES256CBCEncrypt(const unsigned char key[AES256_KEYSIZE], const unsigned char ivIn[AES_BLOCKSIZE], bool padIn);
    ~AES256CBCEncrypt();
    int Encrypt(const unsigned char* data, int size, unsigned char* out) const;

private:
    const AES256Encrypt enc;
    const bool pad;
    unsigned char iv[AES_BLOCKSIZE];
};

class AES256CBCDecrypt
{
public:
    AES256CBCDecrypt(const unsigned char key[AES256_KEYSIZE], const unsigned char ivIn[AES_BLOCKSIZE], bool padIn);
    ~AES256CBCDecrypt();
    int Decrypt(const unsigned char* data, int size, unsigned char* out) const;

private:
    const AES256Decrypt dec;
    const bool pad;
    unsigned char iv[AES_BLOCKSIZE];
};

class AES128CBCEncrypt
{
public:
    AES128CBCEncrypt(const unsigned char key[AES128_KEYSIZE], const unsigned char ivIn[AES_BLOCKSIZE], bool padIn);
    ~AES128CBCEncrypt();
    int Encrypt(const unsigned char* data, int size, unsigned char* out) const;

private:
    const AES128Encrypt enc;
    const bool pad;
    unsigned char iv[AES_BLOCKSIZE];
};

class AES128CBCDecrypt
{
public:
    AES128CBCDecrypt(const unsigned char key[AES128_KEYSIZE], const unsigned char ivIn[AES_BLOCKSIZE], bool padIn);
    ~AES128CBCDecrypt();
    int Decrypt(const unsigned char* data, int size, unsigned char* out) const;

private:
    const AES128Decrypt dec;
    const bool pad;
    unsigned char iv[AES_BLOCKSIZE];
};

#endif // BITCOIN_CRYPTO_AES_H
