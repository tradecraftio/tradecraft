// Copyright (c) 2020-2023 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <consensus/merklerange.h> // for MmrAccumulator
#include <primitives/block.h> // for CBlockHeader
#include <script/standard.h> // for WitnessUnknown
#include <serialize.h> // for Serialize, Unserialize, VARINT
#include <uint256.h> // for uint256

#include <stdint.h> // for uint32_t

struct ShareWitness {
    // A share is committed to at the end of the coinbase transaction, which
    // allows for midstate compression in some use cases.  This is not one of
    // those use cases, as the rest of the transaction is required to validate
    // the coinbase rewards, so the whole coinbase transaction is stored.

    //! The share commitment is possibly aggregated with other commitments in
    //! the block using a Merkle hash map structure.  The key is fixed for the
    //! share chain, so we need only store the skip hash and number of skipped
    //! bits for each level.
    std::vector<std::pair<unsigned char, uint256>> commit;

    //! The coinbase transaction of the block, serialized up to the point of
    //! the commitment.
    std::vector<unsigned char> cb1;
    //! The nLockTime field of the coinbase, the only field after the
    //! commitment.
    uint32_t nLockTime{0};

    //! The path through the Block's transaction Merkle tree to the coinbase
    //! transaction, which is always the left-most leaf.
    std::vector<uint256> branch;

    //! The bitcoin block header fields.  Note that the Merkle root field is
    //! redundant as it can be computed from the coinbase transaction and
    //! Merkle branch, so it is not included here.
    int32_t nVersion{0};
    uint256 hashPrevBlock;
    //! A unique identifier for the chain this share is a part of.
    uint256 share_chain_path;
    uint32_t nTime{0};
    uint32_t nBits{0};
    uint32_t nNonce{0};

    template<typename Stream>
    void Serialize(Stream &s) const {
        // NB: We use Pieter Wuille's VARINT encoding for vector lengths,
        //     which requires doing our own manual serialization of vectors
        //     and arrays.

        // The commitment proof.  The commit branch size can be between 0 and
        // 256, inclusive, so unfortunately we can't just serialize the length
        // as a single byte.
        ::Serialize(s, VARINT(commit.size()));
        for (const auto& pair : commit) {
            ::Serialize(s, pair.first);  // unsigned char
            ::Serialize(s, pair.second); // uint256
        }
        // The coinbase transaction, minus the commitment (which is inserted
        // after cb1 but before nLockTime).
        ::Serialize(s, VARINT(cb1.size()));
        for (const auto& byte : cb1) {
            ::Serialize(s, byte);
        }
        ::Serialize(s, nLockTime);
        // The path to the coinbase transaction.
        ::Serialize(s, VARINT(branch.size()));
        for (const auto& hash : branch) {
            ::Serialize(s, hash);
        }
        // The block header fields, with the share chain path instead of the
        // Merkle root (which is computed from the coinbase transaction and
        // Merkle branch).
        ::Serialize(s, nVersion);
        ::Serialize(s, hashPrevBlock);
        ::Serialize(s, share_chain_path);
        ::Serialize(s, nTime);
        ::Serialize(s, nBits);
        ::Serialize(s, nNonce);
    }

    template<typename Stream>
    void Unserialize(Stream &s) {
        // The commitment proof.
        size_t len = 0;
        ::Unserialize(s, VARINT(len));
        commit.resize(len);
        for (auto& pair : commit) {
            ::Unserialize(s, pair.first);  // unsigned char
            ::Unserialize(s, pair.second); // uint256
        }
        // The coinbase transaction, except for the commitment.
        len = 0;
        ::Unserialize(s, VARINT(len));
        cb1.resize(len);
        for (auto& byte : cb1) {
            ::Unserialize(s, byte);
        }
        ::Unserialize(s, nLockTime);
        // The path to the coinbase transaction.
        len = 0;
        ::Unserialize(s, VARINT(len));
        branch.resize(len);
        for (auto& hash : branch) {
            ::Unserialize(s, hash);
        }
        // The block header fields, with share_chain_path instead for
        // hashMerkleRoot.
        ::Unserialize(s, nVersion);
        ::Unserialize(s, hashPrevBlock);
        ::Unserialize(s, share_chain_path);
        ::Unserialize(s, nTime);
        ::Unserialize(s, nBits);
        ::Unserialize(s, nNonce);
    }
};

void swap(ShareWitness& lhs, ShareWitness& rhs);

struct Share {
    // First we have the header fields which define the share itself.  These
    // have to do with the share chain.

    //! The version of the share header.  This is used to allow for future
    //! extensions to the share format deployed with miner coordination, much
    //! like versionbits with blocks.
    uint32_t version{0};
    //! The target difficulty of this share.
    uint32_t bits{0};
    //! The height of this share in the share chain.
    uint32_t height{0};
    //! The aggregate work done on the share chain up to but not including
    //! this share.
    uint256 total_work;
    //! A Merkle Mountain Range of the previous shares in the share chain.
    MmrAccumulator prev_shares;
    //! The address of the miner who submitted this share.  Typically this
    //! wouldn't actually be an unknown witness type, but rather one of the
    //! existing (and understood) witness types in <script/standard.h>.
    //! However (1) we don't actually need to know the type of the witness
    //! here, and (2) we want to support future witness types too.  So we
    //! treat everything as WitnessUnknown.
    WitnessUnknown miner; // sizeof(WitnessUnknown) == 48 bytes

    // Next we have the witness data, which proves the commitment to the share
    // header within the context of a bitcoin block, as well as the necessary
    // data to verify the share (e.g. the coinbase transaction).

    //! The block commitment data for this share.
    ShareWitness wit;

    SERIALIZE_METHODS(Share, obj) {
        if (s.GetType() & SER_GETHASH) {
            CBlockHeader blkhdr = obj.GetBlockHeader(nullptr);
            READWRITE(blkhdr);
        } else {
            READWRITE(obj.version);
            READWRITE(obj.bits);
            READWRITE(obj.height);
            READWRITE(obj.total_work);
            READWRITE(obj.prev_shares);
            READWRITE(VARINT(obj.miner.version));
            READWRITE(VARINT(obj.miner.length));
            READWRITE(Span(obj.miner.program, obj.miner.length));
            READWRITE(obj.wit);
        }
    }

    //! The block header for this share.
    CBlockHeader GetBlockHeader(bool* mutated) const;

    //! The hash of the share header.
    uint256 GetHash() const { return GetBlockHeader(nullptr).GetHash(); };
};

void swap(Share& lhs, Share& rhs);

// End of File
