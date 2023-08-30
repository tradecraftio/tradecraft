// Copyright (c) 2019-2022 The Bitcoin Core developers
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

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <cstdint>
#include <string>
#include <vector>

/** Parse an HD keypaths like "m/7/0'/2000". */
[[nodiscard]] bool ParseHDKeypath(const std::string& keypath_str, std::vector<uint32_t>& keypath);

/** Write HD keypaths as strings */
std::string WriteHDKeypath(const std::vector<uint32_t>& keypath);
std::string FormatHDKeypath(const std::vector<uint32_t>& path);

#endif // BITCOIN_UTIL_BIP32_H
