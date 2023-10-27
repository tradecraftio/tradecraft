// Copyright (c) 2020 The Bitcoin Core developers
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

#include <checkqueue.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
struct DumbCheck {
    const bool result = false;

    DumbCheck() = default;

    explicit DumbCheck(const bool _result) : result(_result)
    {
    }

    bool operator()() const
    {
        return result;
    }

    void swap(DumbCheck& x)
    {
    }
};
} // namespace

FUZZ_TARGET(checkqueue)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const unsigned int batch_size = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 1024);
    CCheckQueue<DumbCheck> check_queue_1{batch_size};
    CCheckQueue<DumbCheck> check_queue_2{batch_size};
    std::vector<DumbCheck> checks_1;
    std::vector<DumbCheck> checks_2;
    const int size = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 1024);
    for (int i = 0; i < size; ++i) {
        const bool result = fuzzed_data_provider.ConsumeBool();
        checks_1.emplace_back(result);
        checks_2.emplace_back(result);
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        check_queue_1.Add(checks_1);
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        (void)check_queue_1.Wait();
    }

    CCheckQueueControl<DumbCheck> check_queue_control{&check_queue_2};
    if (fuzzed_data_provider.ConsumeBool()) {
        check_queue_control.Add(checks_2);
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        (void)check_queue_control.Wait();
    }
}
