// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#include <shutdown.h>

#include <kernel/context.h>
#include <logging.h>
#include <util/check.h>
#include <util/signalinterrupt.h>

#include <assert.h>
#include <system_error>

void StartShutdown()
{
    try {
        Assert(kernel::g_context)->interrupt();
    } catch (const std::system_error&) {
        LogPrintf("Sending shutdown token failed\n");
        assert(0);
    }
}

void AbortShutdown()
{
    Assert(kernel::g_context)->interrupt.reset();
}

bool ShutdownRequested()
{
    return bool{Assert(kernel::g_context)->interrupt};
}

void WaitForShutdown()
{
    try {
        Assert(kernel::g_context)->interrupt.wait();
    } catch (const std::system_error&) {
        LogPrintf("Reading shutdown token failed\n");
        assert(0);
    }
}
