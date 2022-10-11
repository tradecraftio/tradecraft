// Copyright (c) 2019 The Bitcoin Core developers
// Copyright (c) 2011-2022 The Freicoin Developers
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

#ifndef BITCOIN_UTIL_VECTOR_H
#define BITCOIN_UTIL_VECTOR_H

#include <initializer_list>
#include <type_traits>
#include <vector>

/** Construct a vector with the specified elements.
 *
 * This is preferable over the list initializing constructor of std::vector:
 * - It automatically infers the element type from its arguments.
 * - If any arguments are rvalue references, they will be moved into the vector
 *   (list initialization always copies).
 */
template<typename... Args>
inline std::vector<typename std::common_type<Args...>::type> Vector(Args&&... args)
{
    std::vector<typename std::common_type<Args...>::type> ret;
    ret.reserve(sizeof...(args));
    // The line below uses the trick from https://www.experts-exchange.com/articles/32502/None-recursive-variadic-templates-with-std-initializer-list.html
    (void)std::initializer_list<int>{(ret.emplace_back(std::forward<Args>(args)), 0)...};
    return ret;
}

/** Concatenate two vectors, moving elements. */
template<typename V>
inline V Cat(V v1, V&& v2)
{
    v1.reserve(v1.size() + v2.size());
    for (auto& arg : v2) {
        v1.push_back(std::move(arg));
    }
    return v1;
}

/** Concatenate two vectors. */
template<typename V>
inline V Cat(V v1, const V& v2)
{
    v1.reserve(v1.size() + v2.size());
    for (const auto& arg : v2) {
        v1.push_back(arg);
    }
    return v1;
}

#endif // BITCOIN_UTIL_VECTOR_H
