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

#include <bench/data.h>

namespace benchmark {
namespace data {

#include <bench/data/block136207.raw.h>
const std::vector<uint8_t> block136207{block136207_raw, block136207_raw + sizeof(block136207_raw) / sizeof(block136207_raw[0])};

} // namespace data
} // namespace benchmark
