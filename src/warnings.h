// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
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

#ifndef BITCOIN_WARNINGS_H
#define BITCOIN_WARNINGS_H

#include <stdlib.h>
#include <string>

void SetMiscWarning(const std::string& strWarning);
void SetfLargeWorkForkFound(bool flag);
bool GetfLargeWorkForkFound();
void SetfLargeWorkInvalidChainFound(bool flag);
bool GetfLargeWorkInvalidChainFound();
std::string GetWarnings(const std::string& strFor);

static const bool DEFAULT_TESTSAFEMODE = false;

#endif //  BITCOIN_WARNINGS_H
