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

#ifndef FREICOIN_TEST_UTIL_RANDOM_H
#define FREICOIN_TEST_UTIL_RANDOM_H

#include <consensus/amount.h>
#include <random.h>
#include <uint256.h>

#include <cstdint>

/**
 * This global and the helpers that use it are not thread-safe.
 *
 * If thread-safety is needed, the global could be made thread_local (given
 * that thread_local is supported on all architectures we support) or a
 * per-thread instance could be used in the multi-threaded test.
 */
extern FastRandomContext g_insecure_rand_ctx;

/**
 * Flag to make GetRand in random.h return the same number
 */
extern bool g_mock_deterministic_tests;

enum class SeedRand {
    ZEROS, //!< Seed with a compile time constant of zeros
    SEED,  //!< Call the Seed() helper
};

/** Seed the given random ctx or use the seed passed in via an environment var */
void Seed(FastRandomContext& ctx);

static inline void SeedInsecureRand(SeedRand seed = SeedRand::SEED)
{
    if (seed == SeedRand::ZEROS) {
        g_insecure_rand_ctx = FastRandomContext(/*fDeterministic=*/true);
    } else {
        Seed(g_insecure_rand_ctx);
    }
}

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

#endif // FREICOIN_TEST_UTIL_RANDOM_H
