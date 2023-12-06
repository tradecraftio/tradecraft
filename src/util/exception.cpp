// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2023 The Bitcoin Core developers
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

#include <util/exception.h>

#include <logging.h>
#include <tinyformat.h>

#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>

#ifdef WIN32
#include <windows.h>
#endif // WIN32

static std::string FormatException(const std::exception* pex, std::string_view thread_name)
{
#ifdef WIN32
    char pszModule[MAX_PATH] = "";
    GetModuleFileNameA(nullptr, pszModule, sizeof(pszModule));
#else
    const char* pszModule = "freicoin";
#endif
    if (pex)
        return strprintf(
            "EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, thread_name);
    else
        return strprintf(
            "UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, thread_name);
}

void PrintExceptionContinue(const std::exception* pex, std::string_view thread_name)
{
    std::string message = FormatException(pex, thread_name);
    LogPrintf("\n\n************************\n%s\n", message);
    tfm::format(std::cerr, "\n\n************************\n%s\n", message);
}
