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

#include <node/database_args.h>

#include <common/args.h>
#include <dbwrapper.h>

namespace node {
void ReadDatabaseArgs(const ArgsManager& args, DBOptions& options)
{
    // Settings here apply to all databases (chainstate, blocks, and index
    // databases), but it'd be easy to parse database-specific options by adding
    // a database_type string or enum parameter to this function.
    if (auto value = args.GetBoolArg("-forcecompactdb")) options.force_compact = *value;
}
} // namespace node
