// Copyright (c) 2019 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <core_memusage.h>
#include <pubkey.h>
#include <primitives/block.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <validation.h>
#include <version.h>

#include <cassert>
#include <string>

void initialize()
{
    static const ECCVerifyHandle verify_handle;
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    CBlock block;
    try {
        int nVersion;
        ds >> nVersion;
        ds.SetVersion(nVersion);
        ds >> block;
    } catch (const std::ios_base::failure&) {
        return;
    }
    const Consensus::Params& consensus_params = Params().GetConsensus();
    BlockValidationState validation_state_pow_and_merkle;
    const bool valid_incl_pow_and_merkle = CheckBlock(block, validation_state_pow_and_merkle, consensus_params, /* fCheckPOW= */ true, /* fCheckMerkleRoot= */ true);
    BlockValidationState validation_state_pow;
    const bool valid_incl_pow = CheckBlock(block, validation_state_pow, consensus_params, /* fCheckPOW= */ true, /* fCheckMerkleRoot= */ false);
    BlockValidationState validation_state_merkle;
    const bool valid_incl_merkle = CheckBlock(block, validation_state_merkle, consensus_params, /* fCheckPOW= */ false, /* fCheckMerkleRoot= */ true);
    BlockValidationState validation_state_none;
    const bool valid_incl_none = CheckBlock(block, validation_state_none, consensus_params, /* fCheckPOW= */ false, /* fCheckMerkleRoot= */ false);
    if (valid_incl_pow_and_merkle) {
        assert(valid_incl_pow && valid_incl_merkle && valid_incl_none);
    } else if (valid_incl_merkle || valid_incl_pow) {
        assert(valid_incl_none);
    }
    (void)block.GetHash();
    (void)block.ToString();
    (void)BlockMerkleRoot(block);
    if (!block.vtx.empty()) {
        // TODO: Avoid array index out of bounds error in BlockWitnessMerkleRoot
        //       when block.vtx.empty().
        (void)BlockWitnessMerkleRoot(block);
    }
    (void)GetBlockWeight(block);
    (void)GetWitnessCommitmentIndex(block);
    const size_t raw_memory_size = RecursiveDynamicUsage(block);
    const size_t raw_memory_size_as_shared_ptr = RecursiveDynamicUsage(std::make_shared<CBlock>(block));
    assert(raw_memory_size_as_shared_ptr > raw_memory_size);
}
