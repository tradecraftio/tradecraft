// Copyright (c) 2022 The Bitcoin Core developers
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
#include <pubkey.h>
#include <random.h>
#include <span.h>

#include <array>
#include <cstddef>

static void BIP324_ECDH(benchmark::Bench& bench)
{
    ECC_Start();
    FastRandomContext rng;

    std::array<std::byte, 32> key_data;
    std::array<std::byte, EllSwiftPubKey::size()> our_ellswift_data;
    std::array<std::byte, EllSwiftPubKey::size()> their_ellswift_data;

    rng.fillrand(key_data);
    rng.fillrand(our_ellswift_data);
    rng.fillrand(their_ellswift_data);

    bench.batch(1).unit("ecdh").run([&] {
        CKey key;
        key.Set(UCharCast(key_data.data()), UCharCast(key_data.data()) + 32, true);
        EllSwiftPubKey our_ellswift(our_ellswift_data);
        EllSwiftPubKey their_ellswift(their_ellswift_data);

        auto ret = key.ComputeBIP324ECDHSecret(their_ellswift, our_ellswift, true);

        // To make sure that the computation is not the same on every iteration (ellswift decoding
        // is variable-time), distribute bytes from the shared secret over the 3 inputs. The most
        // important one is their_ellswift, because that one is actually decoded, so it's given most
        // bytes. The data is copied into the middle, so that both halves are affected:
        // - Copy 8 bytes from the resulting shared secret into middle of the private key.
        std::copy(ret.begin(), ret.begin() + 8, key_data.begin() + 12);
        // - Copy 8 bytes from the resulting shared secret into the middle of our ellswift key.
        std::copy(ret.begin() + 8, ret.begin() + 16, our_ellswift_data.begin() + 28);
        // - Copy 16 bytes from the resulting shared secret into the middle of their ellswift key.
        std::copy(ret.begin() + 16, ret.end(), their_ellswift_data.begin() + 24);
    });

    ECC_Stop();
}

BENCHMARK(BIP324_ECDH, benchmark::PriorityLevel::HIGH);
