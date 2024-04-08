// Copyright (c) 2020 The Bitcoin Core developers
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

#include <script/freicoinconsensus.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(script_freicoin_consensus)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::vector<uint8_t> random_bytes_1 = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const std::vector<uint8_t> random_bytes_2 = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const CAmount money = ConsumeMoney(fuzzed_data_provider);
    freicoinconsensus_error err;
    freicoinconsensus_error* err_p = fuzzed_data_provider.ConsumeBool() ? &err : nullptr;
    const unsigned int n_in = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const unsigned int flags = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    assert(freicoinconsensus_version() == FREICOINCONSENSUS_API_VER);
    if ((flags & SCRIPT_VERIFY_WITNESS) != 0 && (flags & SCRIPT_VERIFY_P2SH) == 0) {
        return;
    }
    (void)freicoinconsensus_verify_script(random_bytes_1.data(), random_bytes_1.size(), random_bytes_2.data(), random_bytes_2.size(), n_in, flags, err_p);
    (void)freicoinconsensus_verify_script_with_amount(random_bytes_1.data(), random_bytes_1.size(), money, random_bytes_2.data(), random_bytes_2.size(), n_in, flags, err_p);

    std::vector<UTXO> spent_outputs;
    std::vector<std::vector<unsigned char>> spent_spks;
    if (n_in <= 24386) {
        spent_outputs.reserve(n_in);
        spent_spks.reserve(n_in);
        for (size_t i = 0; i < n_in; ++i) {
            spent_spks.push_back(ConsumeRandomLengthByteVector(fuzzed_data_provider));
            const CAmount value{ConsumeMoney(fuzzed_data_provider)};
            const auto spk_size{static_cast<unsigned>(spent_spks.back().size())};
            spent_outputs.push_back({.scriptPubKey = spent_spks.back().data(), .scriptPubKeySize = spk_size, .value = value});
        }
    }

    const auto spent_outs_size{static_cast<unsigned>(spent_outputs.size())};

    (void)freicoinconsensus_verify_script_with_spent_outputs(
            random_bytes_1.data(), random_bytes_1.size(), money, random_bytes_2.data(), random_bytes_2.size(),
            spent_outputs.data(), spent_outs_size, n_in, flags, err_p);
}
