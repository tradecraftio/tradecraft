// Copyright (c) 2022 The Bitcoin Core developers
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

#ifndef FREICOIN_NODE_CHAINSTATEMANAGER_ARGS_H
#define FREICOIN_NODE_CHAINSTATEMANAGER_ARGS_H

#include <util/result.h>
#include <validation.h>

class ArgsManager;

/** Maximum number of dedicated script-checking threads allowed */
static constexpr int MAX_SCRIPTCHECK_THREADS{15};
/** -par default (number of script-checking threads, 0 = auto) */
static constexpr int DEFAULT_SCRIPTCHECK_THREADS{0};

namespace node {
[[nodiscard]] util::Result<void> ApplyArgsManOptions(const ArgsManager& args, ChainstateManager::Options& opts);
} // namespace node

#endif // FREICOIN_NODE_CHAINSTATEMANAGER_ARGS_H
