// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#include <shutdown.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <logging.h>
#include <node/interface_ui.h>
#include <util/tokenpipe.h>
#include <warnings.h>

#include <assert.h>
#include <atomic>
#ifdef WIN32
#include <condition_variable>
#endif

bool AbortNode(const std::string& strMessage, bilingual_str user_message)
{
    SetMiscWarning(Untranslated(strMessage));
    LogPrintf("*** %s\n", strMessage);
    if (user_message.empty()) {
        user_message = _("A fatal internal error occurred, see debug.log for details");
    }
    InitError(user_message);
    StartShutdown();
    return false;
}

static std::atomic<bool> fRequestShutdown(false);
#ifdef WIN32
/** On windows it is possible to simply use a condition variable. */
std::mutex g_shutdown_mutex;
std::condition_variable g_shutdown_cv;
#else
/** On UNIX-like operating systems use the self-pipe trick.
 */
static TokenPipeEnd g_shutdown_r;
static TokenPipeEnd g_shutdown_w;
#endif

bool InitShutdownState()
{
#ifndef WIN32
    std::optional<TokenPipe> pipe = TokenPipe::Make();
    if (!pipe) return false;
    g_shutdown_r = pipe->TakeReadEnd();
    g_shutdown_w = pipe->TakeWriteEnd();
#endif
    return true;
}

void StartShutdown()
{
#ifdef WIN32
    std::unique_lock<std::mutex> lk(g_shutdown_mutex);
    fRequestShutdown = true;
    g_shutdown_cv.notify_one();
#else
    // This must be reentrant and safe for calling in a signal handler, so using a condition variable is not safe.
    // Make sure that the token is only written once even if multiple threads call this concurrently or in
    // case of a reentrant signal.
    if (!fRequestShutdown.exchange(true)) {
        // Write an arbitrary byte to the write end of the shutdown pipe.
        int res = g_shutdown_w.TokenWrite('x');
        if (res != 0) {
            LogPrintf("Sending shutdown token failed\n");
            assert(0);
        }
    }
#endif
}

void AbortShutdown()
{
    if (fRequestShutdown) {
        // Cancel existing shutdown by waiting for it, this will reset condition flags and remove
        // the shutdown token from the pipe.
        WaitForShutdown();
    }
    fRequestShutdown = false;
}

bool ShutdownRequested()
{
    return fRequestShutdown;
}

void WaitForShutdown()
{
#ifdef WIN32
    std::unique_lock<std::mutex> lk(g_shutdown_mutex);
    g_shutdown_cv.wait(lk, [] { return fRequestShutdown.load(); });
#else
    int res = g_shutdown_r.TokenRead();
    if (res != 'x') {
        LogPrintf("Reading shutdown token failed\n");
        assert(0);
    }
#endif
}
