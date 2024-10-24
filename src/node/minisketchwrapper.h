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

#ifndef FREICOIN_NODE_MINISKETCHWRAPPER_H
#define FREICOIN_NODE_MINISKETCHWRAPPER_H

#include <minisketch.h>

#include <cstddef>
#include <cstdint>

namespace node {
/** Wrapper around Minisketch::Minisketch(32, implementation, capacity). */
Minisketch MakeMinisketch32(size_t capacity);
/** Wrapper around Minisketch::CreateFP. */
Minisketch MakeMinisketch32FP(size_t max_elements, uint32_t fpbits);
} // namespace node

#endif // FREICOIN_NODE_MINISKETCHWRAPPER_H
