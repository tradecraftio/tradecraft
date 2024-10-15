// Copyright (c) The Bitcoin Core developers
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

#include <common/args.h>
#include <i2p.h>
#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <util/fs_helpers.h>
#include <util/threadinterrupt.h>

void initialize_i2p()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET(i2p, .init = initialize_i2p)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    SetMockTime(ConsumeTime(fuzzed_data_provider));

    // Mock CreateSock() to create FuzzedSock.
    auto CreateSockOrig = CreateSock;
    CreateSock = [&fuzzed_data_provider](int, int, int) {
        return std::make_unique<FuzzedSock>(fuzzed_data_provider);
    };

    const fs::path private_key_path = gArgs.GetDataDirNet() / "fuzzed_i2p_private_key";
    const CService addr{in6_addr(IN6ADDR_LOOPBACK_INIT), 7656};
    const Proxy sam_proxy{addr, false};
    CThreadInterrupt interrupt;

    i2p::sam::Session session{private_key_path, sam_proxy, &interrupt};
    i2p::Connection conn;

    if (session.Listen(conn)) {
        if (session.Accept(conn)) {
            try {
                (void)conn.sock->RecvUntilTerminator('\n', 10ms, interrupt, i2p::sam::MAX_MSG_SIZE);
            } catch (const std::runtime_error&) {
            }
        }
    }

    bool proxy_error;

    if (session.Connect(CService{}, conn, proxy_error)) {
        try {
            conn.sock->SendComplete("verack\n", 10ms, interrupt);
        } catch (const std::runtime_error&) {
        }
    }

    fs::remove_all(private_key_path);

    CreateSock = CreateSockOrig;
}
