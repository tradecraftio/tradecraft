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

#include <node/blockmanager_args.h>

#include <common/args.h>
#include <node/blockstorage.h>
#include <tinyformat.h>
#include <util/result.h>
#include <util/translation.h>
#include <validation.h>

#include <cstdint>

namespace node {
util::Result<void> ApplyArgsManOptions(const ArgsManager& args, BlockManager::Options& opts)
{
    // block pruning; get the amount of disk space (in MiB) to allot for block & undo files
    int64_t nPruneArg{args.GetIntArg("-prune", opts.prune_target)};
    if (nPruneArg < 0) {
        return util::Error{_("Prune cannot be configured with a negative value.")};
    }
    uint64_t nPruneTarget{uint64_t(nPruneArg) * 1024 * 1024};
    if (nPruneArg == 1) { // manual pruning: -prune=1
        nPruneTarget = BlockManager::PRUNE_TARGET_MANUAL;
    } else if (nPruneTarget) {
        if (nPruneTarget < MIN_DISK_SPACE_FOR_BLOCK_FILES) {
            return util::Error{strprintf(_("Prune configured below the minimum of %d MiB.  Please use a higher number."), MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024)};
        }
    }
    opts.prune_target = nPruneTarget;

    if (auto value{args.GetBoolArg("-fastprune")}) opts.fast_prune = *value;

    return {};
}
} // namespace node
