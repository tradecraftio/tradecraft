// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
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

#ifndef BITCOIN_UTIL_TIME_H
#define BITCOIN_UTIL_TIME_H

#include <compat.h>

#include <chrono>
#include <stdint.h>
#include <string>

using namespace std::chrono_literals;

void UninterruptibleSleep(const std::chrono::microseconds& n);

/**
 * Helper to count the seconds of a duration.
 *
 * All durations should be using std::chrono and calling this should generally
 * be avoided in code. Though, it is still preferred to an inline t.count() to
 * protect against a reliance on the exact type of t.
 *
 * This helper is used to convert durations before passing them over an
 * interface that doesn't support std::chrono (e.g. RPC, debug log, or the GUI)
 */
constexpr int64_t count_seconds(std::chrono::seconds t) { return t.count(); }
constexpr int64_t count_milliseconds(std::chrono::milliseconds t) { return t.count(); }
constexpr int64_t count_microseconds(std::chrono::microseconds t) { return t.count(); }

using SecondsDouble = std::chrono::duration<double, std::chrono::seconds::period>;

/**
 * Helper to count the seconds in any std::chrono::duration type
 */
inline double CountSecondsDouble(SecondsDouble t) { return t.count(); }

/**
 * DEPRECATED
 * Use either GetTimeSeconds (not mockable) or GetTime<T> (mockable)
 */
int64_t GetTime();

/** Returns the system time (not mockable) */
int64_t GetTimeMillis();
/** Returns the system time (not mockable) */
int64_t GetTimeMicros();
/** Returns the system time (not mockable) */
int64_t GetTimeSeconds(); // Like GetTime(), but not mockable

/**
 * DEPRECATED
 * Use SetMockTime with chrono type
 *
 * @param[in] nMockTimeIn Time in seconds.
 */
void SetMockTime(int64_t nMockTimeIn);

/** For testing. Set e.g. with the setmocktime rpc, or -mocktime argument */
void SetMockTime(std::chrono::seconds mock_time_in);

/** For testing */
std::chrono::seconds GetMockTime();

/** Return system time (or mocked time, if set) */
template <typename T>
T GetTime();

/**
 * ISO 8601 formatting is preferred. Use the FormatISO8601{DateTime,Date}
 * helper functions if possible.
 */
std::string FormatISO8601DateTime(int64_t nTime);
std::string FormatISO8601Date(int64_t nTime);
int64_t ParseISO8601DateTime(const std::string& str);

/**
 * Convert milliseconds to a struct timeval for e.g. select.
 */
struct timeval MillisToTimeval(int64_t nTimeout);

/**
 * Convert milliseconds to a struct timeval for e.g. select.
 */
struct timeval MillisToTimeval(std::chrono::milliseconds ms);

/** Sanity check epoch match normal Unix epoch */
bool ChronoSanityCheck();

#endif // BITCOIN_UTIL_TIME_H
