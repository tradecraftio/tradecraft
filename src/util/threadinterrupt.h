// Copyright (c) 2016-2022 The Bitcoin Core developers
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

#ifndef FREICOIN_UTIL_THREADINTERRUPT_H
#define FREICOIN_UTIL_THREADINTERRUPT_H

#include <sync.h>
#include <threadsafety.h>

#include <atomic>
#include <chrono>
#include <condition_variable>

/**
 * A helper class for interruptible sleeps. Calling operator() will interrupt
 * any current sleep, and after that point operator bool() will return true
 * until reset.
 *
 * This class should not be used in a signal handler. It uses thread
 * synchronization primitives that are not safe to use with signals. If sending
 * an interrupt from a signal handler is necessary, the \ref SignalInterrupt
 * class can be used instead.
 */

class CThreadInterrupt
{
public:
    using Clock = std::chrono::steady_clock;
    CThreadInterrupt();
    explicit operator bool() const;
    void operator()() EXCLUSIVE_LOCKS_REQUIRED(!mut);
    void reset();
    bool sleep_for(Clock::duration rel_time) EXCLUSIVE_LOCKS_REQUIRED(!mut);

private:
    std::condition_variable cond;
    Mutex mut;
    std::atomic<bool> flag;
};

#endif // FREICOIN_UTIL_THREADINTERRUPT_H
