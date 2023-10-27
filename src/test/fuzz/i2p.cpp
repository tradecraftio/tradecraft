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

#include <i2p.h>
#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <threadinterrupt.h>
#include <util/system.h>

void initialize_i2p()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET_INIT(i2p, initialize_i2p)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    // Mock CreateSock() to create FuzzedSock.
    auto CreateSockOrig = CreateSock;
    CreateSock = [&fuzzed_data_provider](const CService&) {
        return std::make_unique<FuzzedSock>(fuzzed_data_provider);
    };

    const CService sam_proxy;
    CThreadInterrupt interrupt;

    i2p::sam::Session sess{gArgs.GetDataDirNet() / "fuzzed_i2p_private_key", sam_proxy, &interrupt};

    i2p::Connection conn;

    if (sess.Listen(conn)) {
        if (sess.Accept(conn)) {
            try {
                (void)conn.sock->RecvUntilTerminator('\n', 10ms, interrupt, i2p::sam::MAX_MSG_SIZE);
            } catch (const std::runtime_error&) {
            }
        }
    }

    const CService to;
    bool proxy_error;

    if (sess.Connect(to, conn, proxy_error)) {
        try {
            conn.sock->SendComplete("verack\n", 10ms, interrupt);
        } catch (const std::runtime_error&) {
        }
    }

    CreateSock = CreateSockOrig;
}
