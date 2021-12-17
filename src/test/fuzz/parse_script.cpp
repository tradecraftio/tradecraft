// Copyright (c) 2009-2019 The Bitcoin Core developers
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

#include <core_io.h>
#include <script/script.h>
#include <test/fuzz/fuzz.h>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string script_string(buffer.begin(), buffer.end());
    try {
        (void)ParseScript(script_string);
    } catch (const std::runtime_error&) {
    }
}
