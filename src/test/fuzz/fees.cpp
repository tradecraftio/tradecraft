// Copyright (c) 2020 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <amount.h>
#include <optional.h>
#include <policy/fees.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CFeeRate minimal_incremental_fee{ConsumeMoney(fuzzed_data_provider)};
    FeeFilterRounder fee_filter_rounder{minimal_incremental_fee};
    while (fuzzed_data_provider.ConsumeBool()) {
        const CAmount current_minimum_fee = ConsumeMoney(fuzzed_data_provider);
        const CAmount rounded_fee = fee_filter_rounder.round(current_minimum_fee);
        assert(MoneyRange(rounded_fee));
    }
}
