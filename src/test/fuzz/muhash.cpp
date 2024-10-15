// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <crypto/muhash.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <vector>

FUZZ_TARGET(muhash)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<uint8_t> data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
    std::vector<uint8_t> data2{ConsumeRandomLengthByteVector(fuzzed_data_provider)};

    MuHash3072 muhash;

    muhash.Insert(data);
    muhash.Insert(data2);

    constexpr uint256 initial_state_hash{"dd5ad2a105c2d29495f577245c357409002329b9f4d6182c0af3dc2f462555c8"};
    uint256 out;
    uint256 out2;
    CallOneOf(
        fuzzed_data_provider,
        [&] {
            // Test that MuHash result is consistent independent of order of operations
            muhash.Finalize(out);

            muhash = MuHash3072();
            muhash.Insert(data2);
            muhash.Insert(data);
            muhash.Finalize(out2);
        },
        [&] {
            // Test that multiplication with the initial state never changes the finalized result
            muhash.Finalize(out);
            MuHash3072 muhash3;
            muhash3 *= muhash;
            muhash3.Finalize(out2);
        },
        [&] {
            // Test that dividing a MuHash by itself brings it back to it's initial state

            // See note about clang + self-assignment in test/uint256_tests.cpp
            #if defined(__clang__)
            #    pragma clang diagnostic push
            #    pragma clang diagnostic ignored "-Wself-assign-overloaded"
            #endif

            muhash /= muhash;

            #if defined(__clang__)
            #    pragma clang diagnostic pop
            #endif

            muhash.Finalize(out);
            out2 = initial_state_hash;
        },
        [&] {
            // Test that removing all added elements brings the object back to it's initial state
            muhash.Remove(data);
            muhash.Remove(data2);
            muhash.Finalize(out);
            out2 = initial_state_hash;
        });
    assert(out == out2);
}
