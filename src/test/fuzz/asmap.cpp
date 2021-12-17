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

#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cstdint>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const Network network = fuzzed_data_provider.PickValueInArray({NET_IPV4, NET_IPV6});
    if (fuzzed_data_provider.remaining_bytes() < 16) {
        return;
    }
    CNetAddr net_addr;
    net_addr.SetRaw(network, fuzzed_data_provider.ConsumeBytes<uint8_t>(16).data());
    std::vector<bool> asmap;
    for (const char cur_byte : fuzzed_data_provider.ConsumeRemainingBytes<char>()) {
        for (int bit = 0; bit < 8; ++bit) {
            asmap.push_back((cur_byte >> bit) & 1);
        }
    }
    (void)net_addr.GetMappedAS(asmap);
}
