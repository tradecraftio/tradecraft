// Copyright (c) 2023 The Bitcoin Core developers
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

#ifndef BITCOIN_UTIL_SIGNALINTERRUPT_H
#define BITCOIN_UTIL_SIGNALINTERRUPT_H

#ifdef WIN32
#include <condition_variable>
#include <mutex>
#else
#include <util/tokenpipe.h>
#endif

#include <atomic>
#include <cstdlib>

namespace util {
/**
 * Helper class that manages an interrupt flag, and allows a thread or
 * signal to interrupt another thread.
 *
 * This class is safe to be used in a signal handler. If sending an interrupt
 * from a signal handler is not necessary, the more lightweight \ref
 * CThreadInterrupt class can be used instead.
 */

class SignalInterrupt
{
public:
    SignalInterrupt();
    explicit operator bool() const;
    void operator()();
    void reset();
    void wait();

private:
    std::atomic<bool> m_flag;

#ifndef WIN32
    // On UNIX-like operating systems use the self-pipe trick.
    TokenPipeEnd m_pipe_r;
    TokenPipeEnd m_pipe_w;
#else
    // On windows use a condition variable, since we don't have any signals there
    std::mutex m_mutex;
    std::condition_variable m_cv;
#endif
};
} // namespace util

#endif // BITCOIN_UTIL_SIGNALINTERRUPT_H
