// Copyright (c) 2019-2021 The Bitcoin Core developers
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

#ifndef FREICOIN_UTIL_CHECK_H
#define FREICOIN_UTIL_CHECK_H

#if defined(HAVE_CONFIG_H)
#include <config/freicoin-config.h>
#endif

#include <tinyformat.h>

#include <stdexcept>

class NonFatalCheckError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/**
 * Throw a NonFatalCheckError when the condition evaluates to false
 *
 * This should only be used
 * - where the condition is assumed to be true, not for error handling or validating user input
 * - where a failure to fulfill the condition is recoverable and does not abort the program
 *
 * For example in RPC code, where it is undesirable to crash the whole program, this can be generally used to replace
 * asserts or recoverable logic errors. A NonFatalCheckError in RPC code is caught and passed as a string to the RPC
 * caller, which can then report the issue to the developers.
 */
#define CHECK_NONFATAL(condition)                                 \
    do {                                                          \
        if (!(condition)) {                                       \
            throw NonFatalCheckError(                             \
                strprintf("Internal bug detected: '%s'\n"         \
                          "%s:%d (%s)\n"                          \
                          "You may report this issue here: %s\n", \
                    (#condition),                                 \
                    __FILE__, __LINE__, __func__,                 \
                    PACKAGE_BUGREPORT));                          \
        }                                                         \
    } while (false)

#if defined(NDEBUG)
#error "Cannot compile without assertions!"
#endif

/** Helper for Assert() */
template <typename T>
T get_pure_r_value(T&& val)
{
    return std::forward<T>(val);
}

/** Identity function. Abort if the value compares equal to zero */
#define Assert(val) ([&]() -> decltype(get_pure_r_value(val)) { auto&& check = (val); assert(#val && check); return std::forward<decltype(get_pure_r_value(val))>(check); }())

/**
 * Assume is the identity function.
 *
 * - Should be used to run non-fatal checks. In debug builds it behaves like
 *   Assert()/assert() to notify developers and testers about non-fatal errors.
 *   In production it doesn't warn or log anything.
 * - For fatal errors, use Assert().
 * - For non-fatal errors in interactive sessions (e.g. RPC or command line
 *   interfaces), CHECK_NONFATAL() might be more appropriate.
 */
#ifdef ABORT_ON_FAILED_ASSUME
#define Assume(val) Assert(val)
#else
#define Assume(val) ([&]() -> decltype(get_pure_r_value(val)) { auto&& check = (val); return std::forward<decltype(get_pure_r_value(val))>(check); }())
#endif

#endif // FREICOIN_UTIL_CHECK_H
