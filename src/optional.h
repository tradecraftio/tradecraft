// Copyright (c) 2017 The Bitcoin Core developers
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

#ifndef FREICOIN_OPTIONAL_H
#define FREICOIN_OPTIONAL_H

#include <utility>

#include <boost/optional.hpp>

//! Substitute for C++17 std::optional
template <typename T>
using Optional = boost::optional<T>;

//! Substitute for C++17 std::make_optional
template <typename T>
Optional<T> MakeOptional(bool condition, T&& value)
{
    return boost::make_optional(condition, std::forward<T>(value));
}

//! Substitute for C++17 std::nullopt
static auto& nullopt = boost::none;

#endif // FREICOIN_OPTIONAL_H
