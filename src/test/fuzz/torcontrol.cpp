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

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <torcontrol.h>

#include <cstdint>
#include <string>
#include <vector>

class DummyTorControlConnection : public TorControlConnection
{
public:
    DummyTorControlConnection() : TorControlConnection{nullptr}
    {
    }

    bool Connect(const std::string&, const ConnectionCB&, const ConnectionCB&)
    {
        return true;
    }

    void Disconnect()
    {
    }

    bool Command(const std::string&, const ReplyHandlerCB&)
    {
        return true;
    }
};

void initialize_torcontrol()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET_INIT(torcontrol, initialize_torcontrol)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    TorController tor_controller;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        TorControlReply tor_control_reply;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_control_reply.code = 250;
            },
            [&] {
                tor_control_reply.code = 510;
            },
            [&] {
                tor_control_reply.code = fuzzed_data_provider.ConsumeIntegral<int>();
            });
        tor_control_reply.lines = ConsumeRandomLengthStringVector(fuzzed_data_provider);
        if (tor_control_reply.lines.empty()) {
            break;
        }
        DummyTorControlConnection dummy_tor_control_connection;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_controller.add_onion_cb(dummy_tor_control_connection, tor_control_reply);
            },
            [&] {
                tor_controller.auth_cb(dummy_tor_control_connection, tor_control_reply);
            },
            [&] {
                tor_controller.authchallenge_cb(dummy_tor_control_connection, tor_control_reply);
            },
            [&] {
                tor_controller.protocolinfo_cb(dummy_tor_control_connection, tor_control_reply);
            });
    }
}
