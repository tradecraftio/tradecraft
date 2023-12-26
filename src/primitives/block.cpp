// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
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

#include <primitives/block.h>

#include <consensus/merkle.h>
#include <hash.h>
#include <tinyformat.h>

#include <array>

std::pair<uint256, uint256> CBlockHeader::GetAuxiliaryHash(const Consensus::Params& params, bool* mutated) const
{
    // Start with the block template hash:
    CBlockHeader blkhdr;
    blkhdr.nVersion       = m_aux_pow.m_commit_version;
    blkhdr.hashPrevBlock  = hashPrevBlock;
    blkhdr.hashMerkleRoot = m_aux_pow.m_commit_hash_merkle_root;
    blkhdr.nTime          = m_aux_pow.m_commit_time;
    blkhdr.nBits          = m_aux_pow.m_commit_bits;
    blkhdr.nNonce         = m_aux_pow.m_commit_nonce;
    uint256 hash = blkhdr.GetHash();

    // The block witholding secret is the final value in the chain.  We commit
    // to the hash of the secret so that the path to the merge mining commitment
    // can be shared with a miner, so that they can make adjustments to the
    // block, without revealing the secret preimage.
    {
        CHashWriter secret(PROTOCOL_VERSION);
        secret << m_aux_pow.m_secret_lo;
        secret << m_aux_pow.m_secret_hi;
        hash = MerkleHash_Sha256Midstate(hash, secret.GetHash());
    }

    // The merge-mining commitment for this chain might be stored alongside
    // other commitments in the form of a Merkle hash map.  We therefore use the
    // branch proof to work our way up to the root value of the Merkle hash map.
    bool invalid = false;
    hash = ComputeMerkleMapRootFromBranch(hash, m_aux_pow.m_commit_branch, params.aux_pow_path, &invalid);
    if (invalid && mutated) {
        *mutated = true;
    }

    // Next we complete the auxiliary block's block-final transaction hash,
    // using the midstate data and commitment root hash.
    {
        CSHA256 midstate(m_aux_pow.m_midstate_hash.begin(),
                        &m_aux_pow.m_midstate_buffer[0],
                         m_aux_pow.m_midstate_length << 3);
        // Write the commitment root hash.
        midstate.Write(hash.begin(), 32);
        // Write the commitment identifier.
        static const std::array<unsigned char, 4> id
            = { 0x4b, 0x4a, 0x49, 0x48 };
        midstate.Write(id.data(), 4);
        // Write the transaction's nLockTime field.
        CDataStream lock_time(SER_NETWORK, PROTOCOL_VERSION);
        lock_time << m_aux_pow.m_aux_lock_time;
        midstate.Write((const unsigned char*)&lock_time[0], lock_time.size());
        // Double SHA-256.
        midstate.Finalize(hash.begin());
        CSHA256()
            .Write(hash.begin(), 32)
            .Finalize(hash.begin());
    }

    // Now we calculate the auxiliary block's Merkle tree root:
    CBlockHeader blkhdraux;
    auto pathmask = ComputeMerklePathAndMask(m_aux_pow.m_aux_branch.size(), m_aux_pow.m_aux_num_txns - 1);
    blkhdraux.hashMerkleRoot = ComputeStableMerkleRootFromBranch(hash, m_aux_pow.m_aux_branch, pathmask.first, pathmask.second, mutated);

    // Complete the auxiliary block header.
    blkhdraux.nVersion      = m_aux_pow.m_aux_version;
    blkhdraux.hashPrevBlock = m_aux_pow.m_aux_hash_prev_block;
    blkhdraux.nTime         = nTime;
    blkhdraux.nBits         = m_aux_pow.m_aux_bits;
    blkhdraux.nNonce        = m_aux_pow.m_aux_nonce;

    // The auxiliary 1st stage hash is the old-style hash of the parent chain
    // block header.
    uint256 aux_hash1 = blkhdraux.GetHash();

    CDataStream aux_block_header(SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_BLOCK_NO_AUX_POW);
    aux_block_header << m_aux_pow.m_secret_lo;
    aux_block_header << m_aux_pow.m_secret_hi;
    aux_block_header << blkhdr;
    aux_block_header << aux_hash1;
    assert(aux_block_header.size() == 128);

    uint256 aux_hash2;
    CSHA256()
        .Write((const unsigned char*)&aux_block_header[0], aux_block_header.size())
        .Midstate(aux_hash2.begin(), nullptr, nullptr);

    return {aux_hash1, aux_hash2};
}

uint256 CBlockHeader::GetHash() const
{
    return (CHashWriter{PROTOCOL_VERSION | SERIALIZE_BLOCK_NO_AUX_POW} << *this).GetHash();
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
