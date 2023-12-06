// Copyright (c) 2022-2023 The Bitcoin Core developers
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

#include <bench/bench.h>

#include <key.h>
#include <random.h>

static void EllSwiftCreate(benchmark::Bench& bench)
{
    ECC_Start();

    CKey key;
    key.MakeNewKey(true);

    uint256 entropy = GetRandHash();

    bench.batch(1).unit("pubkey").run([&] {
        auto ret = key.EllSwiftCreate(MakeByteSpan(entropy));
        /* Use the first 32 bytes of the ellswift encoded public key as next private key. */
        key.Set(UCharCast(ret.data()), UCharCast(ret.data()) + 32, true);
        assert(key.IsValid());
        /* Use the last 32 bytes of the ellswift encoded public key as next entropy. */
        std::copy(ret.begin() + 32, ret.begin() + 64, MakeWritableByteSpan(entropy).begin());
    });

    ECC_Stop();
}

BENCHMARK(EllSwiftCreate, benchmark::PriorityLevel::HIGH);
