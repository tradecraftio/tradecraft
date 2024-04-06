// Copyright (c) 2017-2022 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <cstdlib>
#include <stdint.h>

// classes for ChaCha20 256-bit stream cipher developed by Daniel J. Bernstein
// https://cr.yp.to/chacha/chacha-20080128.pdf */

/** ChaCha20 cipher that only operates on multiples of 64 bytes. */
class ChaCha20Aligned
{
private:
    uint32_t input[12];

public:
    ChaCha20Aligned();

    /** Initialize a cipher with specified 32-byte key. */
    ChaCha20Aligned(const unsigned char* key32);

    /** set 32-byte key. */
    void SetKey32(const unsigned char* key32);

    /** set the 64-bit nonce. */
    void SetIV(uint64_t iv);

    /** set the 64bit block counter (pos seeks to byte position 64*pos). */
    void Seek64(uint64_t pos);

    /** outputs the keystream of size <64*blocks> into <c> */
    void Keystream64(unsigned char* c, size_t blocks);

    /** enciphers the message <input> of length <64*blocks> and write the enciphered representation into <output>
     *  Used for encryption and decryption (XOR)
     */
    void Crypt64(const unsigned char* input, unsigned char* output, size_t blocks);
};

/** Unrestricted ChaCha20 cipher. */
class ChaCha20
{
private:
    ChaCha20Aligned m_aligned;
    unsigned char m_buffer[64] = {0};
    unsigned m_bufleft{0};

public:
    ChaCha20() = default;

    /** Initialize a cipher with specified 32-byte key. */
    ChaCha20(const unsigned char* key32) : m_aligned(key32) {}

    /** set 32-byte key. */
    void SetKey32(const unsigned char* key32)
    {
        m_aligned.SetKey32(key32);
        m_bufleft = 0;
    }

    /** set the 64-bit nonce. */
    void SetIV(uint64_t iv) { m_aligned.SetIV(iv); }

    /** set the 64bit block counter (pos seeks to byte position 64*pos). */
    void Seek64(uint64_t pos)
    {
        m_aligned.Seek64(pos);
        m_bufleft = 0;
    }

    /** outputs the keystream of size <bytes> into <c> */
    void Keystream(unsigned char* c, size_t bytes);

    /** enciphers the message <input> of length <bytes> and write the enciphered representation into <output>
     *  Used for encryption and decryption (XOR)
     */
    void Crypt(const unsigned char* input, unsigned char* output, size_t bytes);
};

#endif // FREICOIN_CRYPTO_CHACHA20_H
