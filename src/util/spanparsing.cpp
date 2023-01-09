// Copyright (c) 2018-2021 The Bitcoin Core developers
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

#include <util/spanparsing.h>

#include <span.h>

#include <string>
#include <vector>

namespace spanparsing {

bool Const(const std::string& str, Span<const char>& sp)
{
    if ((size_t)sp.size() >= str.size() && std::equal(str.begin(), str.end(), sp.begin())) {
        sp = sp.subspan(str.size());
        return true;
    }
    return false;
}

bool Func(const std::string& str, Span<const char>& sp)
{
    if ((size_t)sp.size() >= str.size() + 2 && sp[str.size()] == '(' && sp[sp.size() - 1] == ')' && std::equal(str.begin(), str.end(), sp.begin())) {
        sp = sp.subspan(str.size() + 1, sp.size() - str.size() - 2);
        return true;
    }
    return false;
}

Span<const char> Expr(Span<const char>& sp)
{
    int level = 0;
    auto it = sp.begin();
    while (it != sp.end()) {
        if (*it == '(' || *it == '{') {
            ++level;
        } else if (level && (*it == ')' || *it == '}')) {
            --level;
        } else if (level == 0 && (*it == ')' || *it == '}' || *it == ',')) {
            break;
        }
        ++it;
    }
    Span<const char> ret = sp.first(it - sp.begin());
    sp = sp.subspan(it - sp.begin());
    return ret;
}

std::vector<Span<const char>> Split(const Span<const char>& sp, char sep)
{
    std::vector<Span<const char>> ret;
    auto it = sp.begin();
    auto start = it;
    while (it != sp.end()) {
        if (*it == sep) {
            ret.emplace_back(start, it);
            start = it + 1;
        }
        ++it;
    }
    ret.emplace_back(start, it);
    return ret;
}

} // namespace spanparsing
