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

#include <node/utxo_snapshot.h>

#include <logging.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <txdb.h>
#include <uint256.h>
#include <util/fs.h>
#include <validation.h>

#include <cassert>
#include <cstdio>
#include <optional>
#include <string>

namespace node {

bool WriteSnapshotBaseBlockhash(Chainstate& snapshot_chainstate)
{
    AssertLockHeld(::cs_main);
    assert(snapshot_chainstate.m_from_snapshot_blockhash);

    const std::optional<fs::path> chaindir = snapshot_chainstate.CoinsDB().StoragePath();
    assert(chaindir); // Sanity check that chainstate isn't in-memory.
    const fs::path write_to = *chaindir / node::SNAPSHOT_BLOCKHASH_FILENAME;

    FILE* file{fsbridge::fopen(write_to, "wb")};
    AutoFile afile{file};
    if (afile.IsNull()) {
        LogPrintf("[snapshot] failed to open base blockhash file for writing: %s\n",
                  fs::PathToString(write_to));
        return false;
    }
    afile << *snapshot_chainstate.m_from_snapshot_blockhash;

    if (afile.fclose() != 0) {
        LogPrintf("[snapshot] failed to close base blockhash file %s after writing\n",
                  fs::PathToString(write_to));
        return false;
    }
    return true;
}

std::optional<uint256> ReadSnapshotBaseBlockhash(fs::path chaindir)
{
    if (!fs::exists(chaindir)) {
        LogPrintf("[snapshot] cannot read base blockhash: no chainstate dir "
            "exists at path %s\n", fs::PathToString(chaindir));
        return std::nullopt;
    }
    const fs::path read_from = chaindir / node::SNAPSHOT_BLOCKHASH_FILENAME;
    const std::string read_from_str = fs::PathToString(read_from);

    if (!fs::exists(read_from)) {
        LogPrintf("[snapshot] snapshot chainstate dir is malformed! no base blockhash file "
            "exists at path %s. Try deleting %s and calling loadtxoutset again?\n",
            fs::PathToString(chaindir), read_from_str);
        return std::nullopt;
    }

    uint256 base_blockhash;
    FILE* file{fsbridge::fopen(read_from, "rb")};
    AutoFile afile{file};
    if (afile.IsNull()) {
        LogPrintf("[snapshot] failed to open base blockhash file for reading: %s\n",
            read_from_str);
        return std::nullopt;
    }
    afile >> base_blockhash;

    int64_t position = afile.tell();
    afile.seek(0, SEEK_END);
    if (position != afile.tell()) {
        LogPrintf("[snapshot] warning: unexpected trailing data in %s\n", read_from_str);
    } else if (afile.IsError()) {
        LogPrintf("[snapshot] warning: i/o error reading %s\n", read_from_str);
    }
    return base_blockhash;
}

std::optional<fs::path> FindSnapshotChainstateDir(const fs::path& data_dir)
{
    fs::path possible_dir =
        data_dir / fs::u8path(strprintf("chainstate%s", SNAPSHOT_CHAINSTATE_SUFFIX));

    if (fs::exists(possible_dir)) {
        return possible_dir;
    }
    return std::nullopt;
}

} // namespace node
