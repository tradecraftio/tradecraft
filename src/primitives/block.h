// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#ifndef FREICOIN_PRIMITIVES_BLOCK_H
#define FREICOIN_PRIMITIVES_BLOCK_H

#include <consensus/params.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <util/time.h>
#include <version.h>

static const int SERIALIZE_BLOCK_NO_AUX_POW = 0x20000000;

class AuxProofOfWork {
public:
    // The CBlockHeader structure contains the native proof-of-work header,
    // which in the context of a merge-mined header we call the "compatibility"
    // proof-of-work, since it exists to serve a valid proof-of-work to nodes
    // running non-upgraded versions.
    //
    // To solve an auxiliary proof-of-work, the miner commits to a template for
    // a block within the auxiliary chain (presumably a bitcoin block).  Once a
    // solution is found for the auxiliary proof-of-work, the miner then finds a
    // satisfactory proof-of-work solution under the old rules for effectively
    // the same candidate block.
    //
    // The block template can change in only very specific, clearly enumerated
    // ways when searching for compatibility proof-of-work.  Nonetheless as a
    // result it is possible for every field of the header to be modified except
    // for the hashPrevBlock.  To reconstruct the commitment in the auxiliary
    // chain, we store the unmodified original commitment values.

    // Because of version-rolling, the miner might modify bits of nVersion field
    // of the block header.
    int32_t m_commit_version; // nVersion

    // The miner is allowed to change the scriptSig or nSequence field of the
    // coinbase transaction, which cause hashMerkleRoot to be changed.
    //
    // These fields in the coinbase transaction are the only part of the block's
    // transaction data which the miner is allowed to alter when searching for
    // the compatibility proof-of-work.  The original values are stored in the
    // block itself, and here we store only the root hash of the unmodified
    // transaction Merkle tree.
    uint256 m_commit_hash_merkle_root; // hashMerkleRoot

    // This field is used to store the difficulty adjustment filter prediction
    // for the prior block, rather than any time value.
    uint32_t m_commit_time; // nTime

    // The current difficulty of the auxiliary and compatibility proof-of-work
    // schemes are different and disconnected, of course.  This is the auxiliary
    // proof-of-work difficulty for the merge-mined block (which is different
    // than the auxiliary block's stated difficulty!).
    uint32_t m_commit_bits; // nBits

    // Obviously the compatibility proof-of-work overwrites the nNonce field.
    // In the block template, which otherwise has no use for this field, we use
    // nNonce to store the bias value (1 byte) and difficulty adjustment filter
    // state (3 bytes).
    uint32_t m_commit_nonce; // nNonce

    //
    // The above 5 values, combined with the hashPrevBlock which is shared with
    // the compatibility proof-of-work and therefore stored in CBlockHeader,
    // form the block template commitment.  The next step is to locate the
    // commitment within the auxiliary block's transaction data:
    //

    // To eliminate block witholding attacks, the merge mining is split across
    // two proof of work checks: the first is a regular PoW check using nonce
    // values picked by the miner, and the second is a salted hash keyed by a
    // secret value chosen in advance by the mining pool server and not told
    // to the miner.  In this way the miner is able to check if they have a
    // candidate block worth submitting to the mining pool for credit, but
    // cannot know discover whether the solution they hold is a valid block
    // or not without submitting it to the server.
    //
    // The mining server's secret is a random 128-bit value, which we store as
    // two 64-bit integers.
    uint64_t m_secret_lo;
    uint64_t m_secret_hi;

    //
    // The above fields combine together to generate the merge mining
    // commitment in the following way:
    //
    //   commitment = H(H2(block_header) || H2(m_secret_lo || m_secret_hi))
    //
    // Where H() is the fast Merkle tree hash and H2() is double-SHA256.
    //
    // In other words, the commitment is 1 levels down the left-hand side of a
    // fast Merkle tree, while the right-hand side SKIP hash is a commitment to
    // the block witholding secret.
    //

    // The merge mining commitment is aggregated with other commitments in the
    // auxiliary block using a Merkle hash map structure.  The key is fixed for
    // this chain, so we need only store the skip hash and number of compressed
    // bits for each level.
    std::vector<std::pair<unsigned char, uint256> > m_commit_branch;

    // The merged mining commitment is stored in the last bytes of the last
    // output of the last transaction in the auxiliary block.  This permits us
    // to compress the auxiliary block's final transaction to the midstate of
    // the SHA256 hash of the transaction, right up to the point of the merge
    // mining commitment.  This midstate is the intermediate hash value (or
    // the IV value if a full block of data hasn't been accumulated), the
    // bytes of the next compression block which have been aggregated so far,
    // and a numeric count of the total number of bytes that have been
    // hashed so far (including those in the midstate buffer).
    uint256 m_midstate_hash;
    std::vector<unsigned char> m_midstate_buffer;
    uint32_t m_midstate_length; // Can be 32-bit because the auxiliary
                                // block-final transaction won't be larger
                                // than 2^29 bytes in size (512MB).

    // The nLockTime field of the block-final transaction in the auxiliary
    // block.  This has no consensus meaning in our block chain, but must be
    // part of the proof because it follows the commitment.
    uint32_t m_aux_lock_time;

    //
    // The block-final transaction of the commitment is calculated by
    // initializing a CSHA256 instance with the midstate data, then feeding the
    // hasher the bias, the calculated commitment value (see above), the byte
    // sequence "4b4a4948", followed by m_aux_lock_time.
    //

    // The path to the block-final transaction in the auxiliary block, as
    // calculated by ComputeStableMerkleBranch().
    std::vector<uint256> m_aux_branch;
    uint32_t m_aux_num_txns; // The total number of transactions, which is one
                             // more than the index of the block-final tx.

    //
    // With the block-final hash as the leaf value, the branch is used to
    // calculate the hashMerkleRoot of the auxiliary block's block header.
    // The other fields of the auxiliary block header must be stored
    // explicitly:
    //

    int32_t m_aux_version;         // nVersion
    uint256 m_aux_hash_prev_block; // hashPrevBlock
    uint32_t m_aux_bits;           // nBits
    uint32_t m_aux_nonce;          // nNonce

    //
    // To be compatible with mining bitcoin, the goal of an individual miner
    // must be to find a combination of nonce/timestamp/extranonce values
    // which cause the bitcoin block, the auxiliary block header from our
    // perspective, to have a low hash value.
    //
    // However to prevent block witholding attacks, and to randomize the
    // arrival time of our blocks with respect to bitcoin's (or shard blocks
    // as defined in the proposed Forward Blocks extension), the proof-of-work
    // is split between the auxiliary block hash, ground by the miner, and the
    // per-chain salted hash checked server-side.  The secondary proof-of-work
    // is defined as:
    //
    //   H(H(<genesis hash> || m_secret_lo || m_secret_hi) ||
    //     H(<genesis hash> || m_secret_lo || m_secret_hi) ||
    //     <auxiliary block header>)
    //
    // Where H() in this case is standard SHA-256.  This result must have at
    // least `bias` leading zero bits to be valid merge-mined proof-of-work.
    //

    //
    // Finally, to make this a soft-fork it is required that the winning block
    // have known substitutions of miner-adjustable values which allow it to
    // satisfy the proof-of-work requirements under the old rules to ensure
    // compatibility with non-upgraded noded.  Those substitutions are the
    // values found in CBlockHeader.
    //

public:
    AuxProofOfWork() {
        SetNull();
    }

    /*
     *   - int32_t m_commit_version
     *   - uint256 m_commit_hash_merkle_root
     *   - uint32_t m_commit_time
     *   - uint32_t m_commit_bits
     *   - uint32_t m_commit_nonce
     *   - uint64_t m_secret_lo
     *   - uint64_t m_secret_hi
     *   - std::vector<std::pair<unsigned char, uint256> > m_commit_branch
     *   - uint256 m_midstate_hash
     *   - std::vector<unsigned char> m_midstate_buffer
     *   - VARINT(uint32_t) m_midstate_length
     *   - uint32_t m_aux_lock_time
     *   - std::vector<uint256> m_aux_branch
     *   - VARINT(uint32_t) m_aux_num_txns
     *   - int32_t m_aux_version
     *   - uint256 m_aux_hash_prev_block
     *   - uint32_t m_aux_bits
     *   - uint32_t m_aux_nonce
     */
    template <typename Stream>
    void Serialize(Stream& s) const {
        ::Serialize(s, m_commit_version);
        ::Serialize(s, m_commit_hash_merkle_root);
        ::Serialize(s, m_commit_time);
        ::Serialize(s, m_commit_bits);
        ::Serialize(s, m_commit_nonce);
        ::Serialize(s, m_secret_lo);
        ::Serialize(s, m_secret_hi);
        ::Serialize(s, m_commit_branch);
        ::Serialize(s, m_midstate_hash);
        ::Serialize(s, m_midstate_buffer);
        ::Serialize(s, VARINT(m_midstate_length));
        ::Serialize(s, m_aux_lock_time);
        ::Serialize(s, m_aux_branch);
        {
            uint32_t aux_pos = 0;
            aux_pos = m_aux_num_txns - 1;
            ::Serialize(s, VARINT(aux_pos));
        }
        ::Serialize(s, m_aux_version);
        ::Serialize(s, m_aux_hash_prev_block);
        ::Serialize(s, m_aux_bits);
        ::Serialize(s, m_aux_nonce);
    }

    template <typename Stream>
    void Unserialize(Stream& s) {
        ::Unserialize(s, m_commit_version);
        ::Unserialize(s, m_commit_hash_merkle_root);
        ::Unserialize(s, m_commit_time);
        ::Unserialize(s, m_commit_bits);
        ::Unserialize(s, m_commit_nonce);
        ::Unserialize(s, m_secret_lo);
        ::Unserialize(s, m_secret_hi);
        ::Unserialize(s, m_commit_branch);
        ::Unserialize(s, m_midstate_hash);
        ::Unserialize(s, m_midstate_buffer);
        ::Unserialize(s, VARINT(m_midstate_length));
        ::Unserialize(s, m_aux_lock_time);
        ::Unserialize(s, m_aux_branch);
        {
            uint32_t aux_pos = 0;
            ::Unserialize(s, VARINT(aux_pos));
            m_aux_num_txns = aux_pos + 1;
        }
        ::Unserialize(s, m_aux_version);
        ::Unserialize(s, m_aux_hash_prev_block);
        ::Unserialize(s, m_aux_bits);
        ::Unserialize(s, m_aux_nonce);
    }

    AuxProofOfWork& SetNull() {
        m_commit_version = 0;
        m_commit_hash_merkle_root.SetNull();
        m_commit_time = 0;
        m_commit_bits = 0;
        m_commit_nonce = 0;
        m_secret_lo = 0;
        m_secret_hi = 0;
        m_commit_branch.clear();
        m_midstate_hash.SetNull();
        m_midstate_buffer.clear();
        m_midstate_length = 0;
        m_aux_lock_time = 0;
        m_aux_branch.clear();
        m_aux_num_txns = 0;
        m_aux_version = 0;
        m_aux_hash_prev_block.SetNull();
        m_aux_bits = 0;
        m_aux_nonce = 0;
        return *this;
    }

    bool IsNull() const {
        return (m_aux_num_txns == 0);
    }
};

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // Native header:
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    // Auxiliary proof-of-work header:
    AuxProofOfWork m_aux_pow;

public:
    CBlockHeader()
    {
        SetNull();
    }

    /*
     * Basic block header serialization format:
     * - int32_t nVersion
     * - uint256 hashPrevBlock
     * - uint256 hashMerkleRoot
     * - uint32_t nTime
     * - uint32_t nBits (sign bit clear)
     * - uint32_t nNonce
     *
     * Extended block header serialization format:
     * - int32_t nVersion
     * - uint256 hashPrevBlock
     * - uint256 hashMerkleRoot
     * - uint32_t nTime
     * - uint32_t nBits (sign bit set)
     * - uint32_t nNonce
     * - unsigned char dummy = 0xff
     * - unsigned char flags (!= 0)
     * - if (flags & 1):
     *   - AuxProofOfWork m_aux_pow
     */
    template <typename Stream>
    void Serialize(Stream& s) const {
        ::Serialize(s, this->nVersion);
        ::Serialize(s, hashPrevBlock);
        ::Serialize(s, hashMerkleRoot);
        ::Serialize(s, nTime);
        uint32_t bits = nBits;
        bool extended = !m_aux_pow.IsNull()
                     && !((s.GetVersion() & VERSION_MASK) < AUX_POW_VERSION)
                     && !(s.GetVersion() & SERIALIZE_BLOCK_NO_AUX_POW);
        if (extended) {
            bits |= 1 << 23;
        }
        ::Serialize(s, bits);
        ::Serialize(s, nNonce);
        if (extended) {
            // In a full-block serialization, the block header is followed by
            // a vector containing the transactions of the block.  A value of
            // 0xff would indicate that the number of transactions exceeds
            // 2^32, which is impossible.
            unsigned char dummy = 0xff;
            ::Serialize(s, dummy);
            // The next byte indicates which extended serialization features
            // are present.
            unsigned char flags = 0x01;
            ::Serialize(s, flags);
            // The auxiliary proof of work is the only currently extended
            // block header data serialization presently supported.
            if (   (flags & 1)
                && !((s.GetVersion() & VERSION_MASK) < AUX_POW_VERSION)
                && !(s.GetVersion() & SERIALIZE_BLOCK_NO_AUX_POW))
            {
                flags ^= 1; // clear the aux_pow bit
                ::Serialize(s, m_aux_pow);
            }
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s) {
        ::Unserialize(s, this->nVersion);
        ::Unserialize(s, hashPrevBlock);
        ::Unserialize(s, hashMerkleRoot);
        ::Unserialize(s, nTime);
        bool extended = false;
        uint32_t bits = nBits;
        ::Unserialize(s, bits);
        // The use of the extended serialization format is indicated by setting
        // the sign bit of nBits, a field which can never be negative, and is
        // only stored in signed format for historical reasons.
        extended = bits & 0x00800000;
        nBits    = bits & 0xff7fffff;
        ::Unserialize(s, nNonce);
        if (extended) {
            // In a full-block serialization, the block header is followed by
            // a vector containing the transactions of the block.  A value of
            // 0xff would indicate that the number of transactions exceeds
            // 2^32, which is impossible.
            unsigned char dummy = 0xff;
            ::Unserialize(s, dummy);
            if (dummy != 0xff) {
                throw std::ios_base::failure("Invalid extended block header dummy value");
            }
            // The next byte indicates which extended serialization features
            // are present.
            unsigned char flags = 0x00;
            ::Unserialize(s, flags);
            // The auxiliary proof of work is the only currently extended
            // block header data serialization presently supported.
            if (   (flags & 1)
                && !((s.GetVersion() & VERSION_MASK) < AUX_POW_VERSION)
                && !(s.GetVersion() & SERIALIZE_BLOCK_NO_AUX_POW))
            {
                flags ^= 1; // clear the aux_pow bit
                ::Unserialize(s, m_aux_pow);
            } else {
                // The auxiliary proof-of-work fields are not present.
                m_aux_pow.SetNull();
            }
            // Other flag bits are reserved for future extensions.  This
            // version of the code should never have to parse them as peers
            // will not send any serialized data structures containing them to
            // us.  However if we're started in a data directory of a future
            // release that supports more extended serialization flags then we
            // should crash before doing any damage.
            if (flags) {
                /* Unknown flag in the serialization */
                if (flags & 1) {
                    throw std::ios_base::failure("Unexpected auxiliary proof-of-work");
                }
                throw std::ios_base::failure("Unknown block header optional data");
            }
        } else {
            // Clear the auxiliary proof-of-work if we are reading data in.
            m_aux_pow.SetNull();
        }
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        m_aux_pow.SetNull();
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    // GetAuxiliaryHash() returns a pair of hash values: the first is a hash of
    // a parent chain block header, the second is the block-witholding
    // prevention hash.
    std::pair<uint256, uint256> GetAuxiliaryHash(const Consensus::Params&, bool* mutated = nullptr) const;

    // GetHash() returns the hash seen by non-upgraded nodes, which is also the
    // hash used in the hashPrevBlock field of the next block.  Since this hash
    // commits to the auxiliary 1st stage hash (which also determines the 2nd
    // stage hash), it implicitly contains the merge-mined proof-of-work.
    uint256 GetHash() const;

    NodeSeconds Time() const
    {
        return NodeSeconds{std::chrono::seconds{nTime}};
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    static const uint32_t k_bias_mask = 0xff;

    //! The bias is the number of high-order bits (between 0 and 255) of the
    //! block witholding hash which must be zero for the block to be valid.  The
    //! miner-visible proof-of-work requirement is reduced by the same number of
    //! bits, so that the total proof-of-work requirement remains the same.
    unsigned char GetBias() const
    {
        if (m_aux_pow.IsNull()) {
            return 0;
        }
        // The bias is stuffed into the nNonce field.  This field is replaced in
        // the auxiliary and compatibility block headers, where there is a real
        // proof-of-work requirement, so its only use in a merge mining header
        // is in storing consensus critical values.  The low-order byte stores
        // the bias field, and the remaining higher-order space is used to store
        // the difficulty adjustment filter state.
        return (m_aux_pow.m_commit_nonce & k_bias_mask);
    }

    void SetBias(unsigned char bias)
    {
        // This might not be an auxiliary proof-of-work header, but it doesn't
        // matter.  The bias is stored in the nNonce field, so we don't have to
        // verify that m_aux_pow is valid.
        m_aux_pow.m_commit_nonce = (m_aux_pow.m_commit_nonce & ~k_bias_mask) | bias;
    }
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *(static_cast<CBlockHeader*>(this)) = header;
    }

    SERIALIZE_METHODS(CBlock, obj)
    {
        READWRITE(AsBase<CBlockHeader>(obj), obj.vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        block.m_aux_pow      = m_aux_pow;
        return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    /** Historically CBlockLocator's version field has been written to network
     * streams as the negotiated protocol version and to disk streams as the
     * client version, but the value has never been used.
     *
     * Hard-code to the highest protocol version ever written to a network stream.
     * SerParams can be used if the field requires any meaning in the future,
     **/
    static constexpr int DUMMY_VERSION = 70016;

    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(std::vector<uint256>&& have) : vHave(std::move(have)) {}

    SERIALIZE_METHODS(CBlockLocator, obj)
    {
        int nVersion = DUMMY_VERSION;
        READWRITE(nVersion);
        READWRITE(obj.vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // FREICOIN_PRIMITIVES_BLOCK_H
