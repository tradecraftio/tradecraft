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

#include <test/util/random.h>

#include <logging.h>
#include <random.h>
#include <uint256.h>

#include <cstdlib>
#include <string>

FastRandomContext g_insecure_rand_ctx;

extern void MakeRandDeterministicDANGEROUS(const uint256& seed) noexcept;

void SeedRandomForTest(SeedRand seedtype)
{
    static const std::string RANDOM_CTX_SEED{"RANDOM_CTX_SEED"};

    // Do this once, on the first call, regardless of seedtype, because once
    // MakeRandDeterministicDANGEROUS is called, the output of GetRandHash is
    // no longer truly random. It should be enough to get the seed once for the
    // process.
    static const uint256 ctx_seed = []() {
        // If RANDOM_CTX_SEED is set, use that as seed.
        const char* num = std::getenv(RANDOM_CTX_SEED.c_str());
        if (num) return uint256S(num);
        // Otherwise use a (truly) random value.
        return GetRandHash();
    }();

    const uint256& seed{seedtype == SeedRand::SEED ? ctx_seed : uint256::ZERO};
    LogPrintf("%s: Setting random seed for current tests to %s=%s\n", __func__, RANDOM_CTX_SEED, seed.GetHex());
    MakeRandDeterministicDANGEROUS(seed);
    g_insecure_rand_ctx.Reseed(GetRandHash());
}
