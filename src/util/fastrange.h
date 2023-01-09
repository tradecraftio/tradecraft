// Copyright (c) 2018-2020 The Bitcoin Core developers
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

#ifndef FREICOIN_UTIL_FASTRANGE_H
#define FREICOIN_UTIL_FASTRANGE_H

#include <cstdint>

/* This file offers implementations of the fast range reduction technique described
 * in https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
 *
 * In short, they take an integer x and a range n, and return the upper bits of
 * (x * n). If x is uniformly distributed over its domain, the result is as close to
 * uniformly distributed over [0, n) as (x mod n) would be, but significantly faster.
 */

/** Fast range reduction with 32-bit input and 32-bit range. */
static inline uint32_t FastRange32(uint32_t x, uint32_t n)
{
    return (uint64_t{x} * n) >> 32;
}

/** Fast range reduction with 64-bit input and 64-bit range. */
static inline uint64_t FastRange64(uint64_t x, uint64_t n)
{
#ifdef __SIZEOF_INT128__
    return (static_cast<unsigned __int128>(x) * static_cast<unsigned __int128>(n)) >> 64;
#else
    // To perform the calculation on 64-bit numbers without losing the
    // result to overflow, split the numbers into the most significant and
    // least significant 32 bits and perform multiplication piece-wise.
    //
    // See: https://stackoverflow.com/a/26855440
    const uint64_t x_hi = x >> 32;
    const uint64_t x_lo = x & 0xFFFFFFFF;
    const uint64_t n_hi = n >> 32;
    const uint64_t n_lo = n & 0xFFFFFFFF;

    const uint64_t ac = x_hi * n_hi;
    const uint64_t ad = x_hi * n_lo;
    const uint64_t bc = x_lo * n_hi;
    const uint64_t bd = x_lo * n_lo;

    const uint64_t mid34 = (bd >> 32) + (bc & 0xFFFFFFFF) + (ad & 0xFFFFFFFF);
    const uint64_t upper64 = ac + (bc >> 32) + (ad >> 32) + (mid34 >> 32);
    return upper64;
#endif
}

#endif // FREICOIN_UTIL_FASTRANGE_H
