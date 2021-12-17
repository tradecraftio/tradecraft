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

#include <memusage.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <version.h>

#include <cassert>
#include <cstdint>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    {
        const double d = fuzzed_data_provider.ConsumeFloatingPoint<double>();
        (void)memusage::DynamicUsage(d);
        assert(ser_uint64_to_double(ser_double_to_uint64(d)) == d);

        CDataStream stream(SER_NETWORK, INIT_PROTO_VERSION);
        stream << d;
        double d_deserialized;
        stream >> d_deserialized;
        assert(d == d_deserialized);
    }

    {
        const float f = fuzzed_data_provider.ConsumeFloatingPoint<float>();
        (void)memusage::DynamicUsage(f);
        assert(ser_uint32_to_float(ser_float_to_uint32(f)) == f);

        CDataStream stream(SER_NETWORK, INIT_PROTO_VERSION);
        stream << f;
        float f_deserialized;
        stream >> f_deserialized;
        assert(f == f_deserialized);
    }
}
