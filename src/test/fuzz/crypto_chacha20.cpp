// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_chacha20)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    ChaCha20 chacha20;
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(16, 32));
        chacha20 = ChaCha20{key.data(), key.size()};
    }
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(16, 32));
                chacha20.SetKey(key.data(), key.size());
            },
            [&] {
                chacha20.SetIV(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                chacha20.Seek(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                std::vector<uint8_t> output(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                chacha20.Keystream(output.data(), output.size());
            },
            [&] {
                std::vector<uint8_t> output(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                const std::vector<uint8_t> input = ConsumeFixedLengthByteVector(fuzzed_data_provider, output.size());
                chacha20.Crypt(input.data(), output.data(), input.size());
            });
    }
}
