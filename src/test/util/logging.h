// Copyright (c) 2019-2020 The Bitcoin Core developers
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

#ifndef FREICOIN_TEST_UTIL_LOGGING_H
#define FREICOIN_TEST_UTIL_LOGGING_H

#include <util/macros.h>

#include <functional>
#include <list>
#include <string>

class DebugLogHelper
{
    const std::string m_message;
    bool m_found{false};
    std::list<std::function<void(const std::string&)>>::iterator m_print_connection;

    //! Custom match checking function.
    //!
    //! Invoked with pointers to lines containing matching strings, and with
    //! null if check_found() is called without any successful match.
    //!
    //! Can return true to enable default DebugLogHelper behavior of:
    //! (1) ending search after first successful match, and
    //! (2) raising an error in check_found if no match was found
    //! Can return false to do the opposite in either case.
    using MatchFn = std::function<bool(const std::string* line)>;
    MatchFn m_match;

    void check_found();

public:
    explicit DebugLogHelper(std::string message, MatchFn match = [](const std::string*){ return true; });
    ~DebugLogHelper() { check_found(); }
};

#define ASSERT_DEBUG_LOG(message) DebugLogHelper PASTE2(debugloghelper, __COUNTER__)(message)

#endif // FREICOIN_TEST_UTIL_LOGGING_H
