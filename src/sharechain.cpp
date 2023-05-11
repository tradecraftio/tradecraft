// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sharechain.h>

#include <consensus/merkle.h> // for ComputeMerkleMapRootFromBranch, ComputeMerkleRootFromBranch
#include <hash.h> // for CHash256, CHashWriter
#include <primitives/block.h> // for CBlockHeader
#include <streams.h> // for CDataStream
#include <span.h> // for Span, MakeUCharSpan

#include <array> // for std::array
#include <utility> // for std::swap

void swap(ShareWitness& lhs, ShareWitness& rhs) {
    using std::swap; // for ADL
    swap(lhs.commit, rhs.commit);
    swap(lhs.cb1, rhs.cb1);
    swap(lhs.nLockTime, rhs.nLockTime);
    swap(lhs.branch, rhs.branch);
    swap(lhs.nVersion, rhs.nVersion);
    swap(lhs.hashPrevBlock, rhs.hashPrevBlock);
    swap(lhs.share_chain_path, rhs.share_chain_path);
    swap(lhs.nTime, rhs.nTime);
    swap(lhs.nBits, rhs.nBits);
    swap(lhs.nNonce, rhs.nNonce);
}

void swap(Share& lhs, Share& rhs) {
    using std::swap; // for ADL
    swap(lhs.version, rhs.version);
    swap(lhs.bits, rhs.bits);
    swap(lhs.height, rhs.height);
    swap(lhs.total_work, rhs.total_work);
    swap(lhs.prev_shares, rhs.prev_shares);
    swap(lhs.miner, rhs.miner);
    swap(lhs.wit, rhs.wit);
}

CBlockHeader Share::GetBlockHeader(bool *mutated) const {
    if (mutated) {
        *mutated = false;
    }

    // Compute the hash of the share header.
    CHashWriter ss(PROTOCOL_VERSION);
    ss << version;
    ss << bits;
    ss << height;
    ss << total_work;
    // When being hashed, we include only the root hash of the Merkle
    // mountain range structure, which has the advantage of making the
    // share header a fixed sized structure.
    ss << prev_shares.GetHash();
    ss << COMPACTSIZE(miner.GetWitnessVersion()); // will always be single byte
    ss << miner.GetWitnessProgram();
    uint256 hash = ss.GetHash();

    // Compute the commitment root hash.
    // The share chain commitment might be stored alongside other
    // commitments in the form of a Merkle hash map.  We therefore use
    // the branch proof to work our way up to the root value of the
    // Merkle hash map.
    bool invalid = false;
    hash = ComputeMerkleMapRootFromBranch(hash, wit.commit, wit.share_chain_path, &invalid);
    if (invalid && mutated) {
        *mutated = true;
    }

    // Calculate hash of coinbase transaction.
    CHash256 h;
    // Write the first part of the coinbase transaction.
    h.Write(MakeUCharSpan(wit.cb1));
    // Write the commitment root hash.
    h.Write(MakeUCharSpan(hash));
    // Write the commitment identifier.
    static const std::array<unsigned char, 4> id
        = { 0x4b, 0x4a, 0x49, 0x48 };
    h.Write(MakeUCharSpan(id));
    // Write the rest of the coinbase transaction.
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << wit.nLockTime;
        h.Write(MakeUCharSpan(ds));
    }
    hash = ss.GetHash();

    // Calculate hashMerkleRoot for the block header.
    hash = ComputeMerkleRootFromBranch(hash, wit.branch, 0);

    // Write the block header fields.
    CBlockHeader blkhdr;
    blkhdr.nVersion = wit.nVersion;
    blkhdr.hashPrevBlock = wit.hashPrevBlock;
    blkhdr.hashMerkleRoot = hash;
    blkhdr.nTime = wit.nTime;
    blkhdr.nBits = wit.nBits;
    blkhdr.nNonce = wit.nNonce;
    return blkhdr;
}

// End of File
