// Copyright (c) 2021 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#ifndef FREICOIN_UTIL_OVERLOADED_H
#define FREICOIN_UTIL_OVERLOADED_H

namespace util {
//! Overloaded helper for std::visit. This helper and std::visit in general are
//! useful to write code that switches on a variant type. Unlike if/else-if and
//! switch/case statements, std::visit will trigger compile errors if there are
//! unhandled cases.
//!
//! Implementation comes from and example usage can be found at
//! https://en.cppreference.com/w/cpp/utility/variant/visit#Example
template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };

//! Explicit deduction guide (not needed as of C++20)
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;
} // namespace util

#endif // FREICOIN_UTIL_OVERLOADED_H
