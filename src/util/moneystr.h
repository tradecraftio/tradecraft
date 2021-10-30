// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
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

/**
 * Money parsing/formatting utilities.
 */
#ifndef FREICOIN_UTIL_MONEYSTR_H
#define FREICOIN_UTIL_MONEYSTR_H

#include <amount.h>
#include <attributes.h>

#include <cstdint>
#include <string>

/* Do not use these functions to represent or parse monetary amounts to or from
 * JSON but use AmountFromValue and ValueFromAmount for that.
 */
std::string FormatMoney(const CAmount& n);
NODISCARD bool ParseMoney(const std::string& str, CAmount& nRet);
NODISCARD bool ParseMoney(const char* pszIn, CAmount& nRet);

#endif // FREICOIN_UTIL_MONEYSTR_H
