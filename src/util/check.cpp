// Copyright (c) 2022 The Bitcoin Core developers
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

#include <util/check.h>

#if defined(HAVE_CONFIG_H)
#include <config/freicoin-config.h>
#endif

#include <clientversion.h>
#include <tinyformat.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

std::string StrFormatInternalBug(std::string_view msg, std::string_view file, int line, std::string_view func)
{
    return strprintf("Internal bug detected: %s\n%s:%d (%s)\n"
                     "%s %s\n"
                     "Please report this issue here: %s\n",
                     msg, file, line, func, PACKAGE_NAME, FormatFullVersion(), PACKAGE_BUGREPORT);
}

NonFatalCheckError::NonFatalCheckError(std::string_view msg, std::string_view file, int line, std::string_view func)
    : std::runtime_error{StrFormatInternalBug(msg, file, line, func)}
{
}

void assertion_fail(std::string_view file, int line, std::string_view func, std::string_view assertion)
{
    auto str = strprintf("%s:%s %s: Assertion `%s' failed.\n", file, line, func, assertion);
    fwrite(str.data(), 1, str.size(), stderr);
    std::abort();
}
