// Copyright (c) 2022 The Bitcoin Core developers
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

#include <node/mempool_persist_args.h>

#include <util/fs.h>
#include <util/system.h>
#include <validation.h>

namespace node {

bool ShouldPersistMempool(const ArgsManager& argsman)
{
    return argsman.GetBoolArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL);
}

fs::path MempoolPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / "mempool.dat";
}

} // namespace node
