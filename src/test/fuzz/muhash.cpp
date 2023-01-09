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

#include <crypto/muhash.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <vector>

FUZZ_TARGET(muhash)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<uint8_t> data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    std::vector<uint8_t> data2 = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    if (data.empty()) {
        data.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096), fuzzed_data_provider.ConsumeIntegral<uint8_t>());
    }
    if (data2.empty()) {
        data2.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096), fuzzed_data_provider.ConsumeIntegral<uint8_t>());
    }

    data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    data2 = ConsumeRandomLengthByteVector(fuzzed_data_provider);

    MuHash3072 muhash;

    // Test that MuHash result is consistent independent of order of operations
    muhash.Insert(data);
    muhash.Insert(data2);

    uint256 out;
    muhash.Finalize(out);

    muhash = MuHash3072();
    muhash.Insert(data2);
    muhash.Insert(data);

    uint256 out2;
    muhash.Finalize(out2);

    assert(out == out2);
    MuHash3072 muhash3;
    muhash3 *= muhash;
    uint256 out3;
    muhash3.Finalize(out3);
    assert(out == out3);

    // Test that removing all added elements brings the object back to it's initial state
    muhash /= muhash;
    muhash.Finalize(out);

    MuHash3072 muhash2;
    muhash2.Finalize(out2);

    assert(out == out2);

    muhash3.Remove(data);
    muhash3.Remove(data2);
    muhash3.Finalize(out3);
    assert(out == out3);
}
