
// Copyright (c) 2023 The Bitcoin Core developers
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

#ifndef BITCOIN_NODE_BLOCKMANAGER_ARGS_H
#define BITCOIN_NODE_BLOCKMANAGER_ARGS_H

#include <node/blockstorage.h>

#include <optional>

class ArgsManager;
struct bilingual_str;

namespace node {
std::optional<bilingual_str> ApplyArgsManOptions(const ArgsManager& args, BlockManager::Options& opts);
} // namespace node

#endif // BITCOIN_NODE_BLOCKMANAGER_ARGS_H
