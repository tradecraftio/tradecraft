// Copyright (c) 2021 The Bitcoin Core developers
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
//
#ifndef FREICOIN_TEST_UTIL_CHAINSTATE_H
#define FREICOIN_TEST_UTIL_CHAINSTATE_H

#include <clientversion.h>
#include <fs.h>
#include <logging.h>
#include <node/context.h>
#include <node/utxo_snapshot.h>
#include <rpc/blockchain.h>
#include <validation.h>

#include <univalue.h>

const auto NoMalleation = [](AutoFile& file, node::SnapshotMetadata& meta){};

/**
 * Create and activate a UTXO snapshot, optionally providing a function to
 * malleate the snapshot.
 */
template<typename F = decltype(NoMalleation)>
static bool
CreateAndActivateUTXOSnapshot(node::NodeContext& node, const fs::path root, F malleation = NoMalleation)
{
    // Write out a snapshot to the test's tempdir.
    //
    int height;
    WITH_LOCK(::cs_main, height = node.chainman->ActiveHeight());
    fs::path snapshot_path = root / fs::u8path(tfm::format("test_snapshot.%d.dat", height));
    FILE* outfile{fsbridge::fopen(snapshot_path, "wb")};
    AutoFile auto_outfile{outfile};

    UniValue result = CreateUTXOSnapshot(
        node, node.chainman->ActiveChainstate(), auto_outfile, snapshot_path, snapshot_path);
    LogPrintf(
        "Wrote UTXO snapshot to %s: %s", fs::PathToString(snapshot_path.make_preferred()), result.write());

    // Read the written snapshot in and then activate it.
    //
    FILE* infile{fsbridge::fopen(snapshot_path, "rb")};
    AutoFile auto_infile{infile};
    node::SnapshotMetadata metadata;
    auto_infile >> metadata;

    malleation(auto_infile, metadata);

    return node.chainman->ActivateSnapshot(auto_infile, metadata, /*in_memory=*/true);
}


#endif // FREICOIN_TEST_UTIL_CHAINSTATE_H
