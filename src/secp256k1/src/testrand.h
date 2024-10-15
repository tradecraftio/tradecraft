/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Copyright (c) 2018-2024 The Freicoin Developers                     *
 *                                                                     *
 * This program is free software: you can redistribute it and/or       *
 * modify it under the terms of version 3 of the GNU Affero General    *
 * Public License as published by the Free Software Foundation.        *
 *                                                                     *
 * This program is distributed in the hope that it will be useful,     *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Affero General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Affero General Public    *
 * License along with this program.  If not, see                       *
 * <https://www.gnu.org/licenses/>.                                    *
 ***********************************************************************/

#ifndef SECP256K1_TESTRAND_H
#define SECP256K1_TESTRAND_H

#include "util.h"

/* A non-cryptographic RNG used only for test infrastructure. */

/** Seed the pseudorandom number generator for testing. */
SECP256K1_INLINE static void testrand_seed(const unsigned char *seed16);

/** Generate a pseudorandom number in the range [0..2**32-1]. */
SECP256K1_INLINE static uint32_t testrand32(void);

/** Generate a pseudorandom number in the range [0..2**64-1]. */
SECP256K1_INLINE static uint64_t testrand64(void);

/** Generate a pseudorandom number in the range [0..2**bits-1]. Bits must be 1 or
 *  more. */
SECP256K1_INLINE static uint64_t testrand_bits(int bits);

/** Generate a pseudorandom number in the range [0..range-1]. */
static uint32_t testrand_int(uint32_t range);

/** Generate a pseudorandom 32-byte array. */
static void testrand256(unsigned char *b32);

/** Generate a pseudorandom 32-byte array with long sequences of zero and one bits. */
static void testrand256_test(unsigned char *b32);

/** Generate pseudorandom bytes with long sequences of zero and one bits. */
static void testrand_bytes_test(unsigned char *bytes, size_t len);

/** Flip a single random bit in a byte array */
static void testrand_flip(unsigned char *b, size_t len);

/** Initialize the test RNG using (hex encoded) array up to 16 bytes, or randomly if hexseed is NULL. */
static void testrand_init(const char* hexseed);

/** Print final test information. */
static void testrand_finish(void);

#endif /* SECP256K1_TESTRAND_H */
