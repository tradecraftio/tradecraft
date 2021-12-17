// Copyright (c) 2019 The Bitcoin Core developers
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

#include <chainparams.h>
#include <net.h>
#include <protocol.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

void initialize()
{
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    V1TransportDeserializer deserializer{Params().MessageStart(), SER_NETWORK, INIT_PROTO_VERSION};
    const char* pch = (const char*)buffer.data();
    size_t n_bytes = buffer.size();
    while (n_bytes > 0) {
        const int handled = deserializer.Read(pch, n_bytes);
        if (handled < 0) {
            break;
        }
        pch += handled;
        n_bytes -= handled;
        if (deserializer.Complete()) {
            const int64_t m_time = std::numeric_limits<int64_t>::max();
            const CNetMessage msg = deserializer.GetMessage(Params().MessageStart(), m_time);
            assert(msg.m_command.size() <= CMessageHeader::COMMAND_SIZE);
            assert(msg.m_raw_message_size <= buffer.size());
            assert(msg.m_raw_message_size == CMessageHeader::HEADER_SIZE + msg.m_message_size);
            assert(msg.m_time == m_time);
            if (msg.m_valid_header) {
                assert(msg.m_valid_netmagic);
            }
            if (!msg.m_valid_netmagic) {
                assert(!msg.m_valid_header);
            }
        }
    }
}
