// Copyright (c) 2023 The Bitcoin Core developers
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

#ifndef BITCOIN_TEST_UTIL_RANDOM_H
#define BITCOIN_TEST_UTIL_RANDOM_H

#include <consensus/amount.h>
#include <random.h>
#include <uint256.h>

#include <cstdint>

/**
 * This global and the helpers that use it are not thread-safe.
 *
 * If thread-safety is needed, a per-thread instance could be
 * used in the multi-threaded test.
 */
extern FastRandomContext g_insecure_rand_ctx;

enum class SeedRand {
    ZEROS, //!< Seed with a compile time constant of zeros
    SEED,  //!< Use (and report) random seed from environment, or a (truly) random one.
};

/** Seed the RNG for testing. This affects all randomness, except GetStrongRandBytes(). */
void SeedRandomForTest(SeedRand seed = SeedRand::SEED);

static inline uint32_t InsecureRand32()
{
    return g_insecure_rand_ctx.rand32();
}

static inline uint256 InsecureRand256()
{
    return g_insecure_rand_ctx.rand256();
}

static inline uint64_t InsecureRandBits(int bits)
{
    return g_insecure_rand_ctx.randbits(bits);
}

static inline uint64_t InsecureRandRange(uint64_t range)
{
    return g_insecure_rand_ctx.randrange(range);
}

static inline bool InsecureRandBool()
{
    return g_insecure_rand_ctx.randbool();
}

static inline CAmount InsecureRandMoneyAmount()
{
    return static_cast<CAmount>(InsecureRandRange(MAX_MONEY + 1));
}

#endif // BITCOIN_TEST_UTIL_RANDOM_H
