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

#ifndef BITCOIN_UTIL_CHECK_H
#define BITCOIN_UTIL_CHECK_H

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
 * For example in RPC code, where it is undersirable to crash the whole program, this can be generally used to replace
 * asserts or recoverable logic errors. A NonFatalCheckError in RPC code is caught and passed as a string to the RPC
 * caller, which can then report the issue to the developers.
 */
#define CHECK_NONFATAL(condition)                                 \
    do {                                                          \
        if (!(condition)) {                                       \
            throw NonFatalCheckError(                             \
                strprintf("%s:%d (%s)\n"                          \
                          "Internal bug detected: '%s'\n"         \
                          "You may report this issue here: %s\n", \
                    __FILE__, __LINE__, __func__,                 \
                    (#condition),                                 \
                    PACKAGE_BUGREPORT));                          \
        }                                                         \
    } while (false)

#endif // BITCOIN_UTIL_CHECK_H
