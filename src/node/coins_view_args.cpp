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

#include <node/coins_view_args.h>

#include <txdb.h>
#include <util/system.h>

namespace node {
void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options)
{
    if (auto value = args.GetIntArg("-dbbatchsize")) options.batch_write_bytes = *value;
    if (auto value = args.GetIntArg("-dbcrashratio")) options.simulate_crash_ratio = *value;
}
} // namespace node
