// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include "merkle.h"
#include "hash.h"
#include "streams.h"
#include "utilstrencodings.h"

/*     WARNING! If you're reading this because you're learning about crypto
       and/or designing a new system that will use merkle trees, keep in mind
       that the following merkle tree algorithm has a serious flaw related to
       duplicate txids, resulting in a vulnerability (CVE-2012-2459).

       The reason is that if the number of hashes in the list at a given time
       is odd, the last one is duplicated before computing the next level (which
       is unusual in Merkle trees). This results in certain sequences of
       transactions leading to the same merkle root. For example, these two
       trees:

                    A               A
                  /  \            /   \
                B     C         B       C
               / \    |        / \     / \
              D   E   F       D   E   F   F
             / \ / \ / \     / \ / \ / \ / \
             1 2 3 4 5 6     1 2 3 4 5 6 5 6

       for transaction lists [1,2,3,4,5,6] and [1,2,3,4,5,6,5,6] (where 5 and
       6 are repeated) result in the same root hash A (because the hash of both
       of (F) and (F,F) is C).

       The vulnerability results from being able to send a block with such a
       transaction list, with the same merkle root, and the same block hash as
       the original without duplication, resulting in failed validation. If the
       receiving node proceeds to mark that block as permanently invalid
       however, it will fail to accept further unmodified (and thus potentially
       valid) versions of the same block. We defend against this by detecting
       the case where we would hash two identical hashes at the end of the list
       together, and treating that identically to the block having an invalid
       merkle root. Assuming no double-SHA256 collisions, this will detect all
       known ways of changing the transactions without affecting the merkle
       root.
*/

typedef unsigned merklecomputationopts;
static const merklecomputationopts MERKLE_COMPUTATION_MUTABLE = 0x1;
static const merklecomputationopts MERKLE_COMPUTATION_FAST    = 0x2;
static const merklecomputationopts MERKLE_COMPUTATION_STABLE  = 0x4;

void MerkleHash_Hash256(uint256& parent, const uint256& left, const uint256& right) {
    CHash256().Write(left.begin(), 32).Write(right.begin(), 32).Finalize(parent.begin());
}

/* Calculated by using standard FIPS-180 SHA-256 to produce the digest of the
 * empty string / zero-length byte array, then feeding the resulting digest into
 * SHA-256 twice in order to fill a block and trigger a compression round.  The
 * midstate is then extracted and used as our hash value.  Expressed with the
 * pure-Python sha256 package, this would be something like the following:
 *
 *   iv = sha256(sha256().digest() + sha256().digest()).state[0]
 */
static unsigned char _MidstateIV[32] =
    { 0x1e, 0x4e, 0x0f, 0x95, 0x5a, 0x4b, 0xc8, 0x1c,
      0x08, 0xc8, 0xaf, 0x1c, 0x94, 0xf3, 0x4b, 0x9d,
      0x0a, 0xf2, 0xf4, 0x50, 0xdc, 0x24, 0xa3, 0xbc,
      0xef, 0x98, 0x31, 0x8f, 0xaf, 0x5e, 0x25, 0x06 };
void MerkleHash_Sha256Midstate(uint256& parent, const uint256& left, const uint256& right) {
    CSHA256(_MidstateIV).Write(left.begin(), 32).Write(right.begin(), 32).Midstate(parent.begin(), nullptr, nullptr);
}

/* This implements a constant-space merkle root/path calculator, limited to 2^32 leaves. */
static void MerkleComputation(const std::vector<uint256>& leaves, uint256* proot, bool* pmutated, uint32_t branchpos, std::vector<uint256>* pbranch, merklecomputationopts flags) {
    if (pbranch) pbranch->clear();
    if (leaves.size() == 0) {
        if (pmutated) *pmutated = false;
        if (proot) *proot = uint256();
        return;
    }
    bool is_mutable = (flags & MERKLE_COMPUTATION_MUTABLE) != 0;
    bool is_stable = (flags & MERKLE_COMPUTATION_STABLE) != 0;
    auto MerkleHash = MerkleHash_Hash256;
    if (flags & MERKLE_COMPUTATION_FAST) {
        MerkleHash = MerkleHash_Sha256Midstate;
    }
    bool mutated = false;
    // count is the number of leaves processed so far.
    uint32_t count = 0;
    // inner is an array of eagerly computed subtree hashes, indexed by tree
    // level (0 being the leaves).
    // For example, when count is 25 (11001 in binary), inner[4] is the hash of
    // the first 16 leaves, inner[3] of the next 8 leaves, and inner[0] equal to
    // the last leaf. The other inner entries are undefined.
    uint256 inner[32];
    // Which position in inner is a hash that depends on the matching leaf.
    int matchlevel = -1;
    // First process all leaves into 'inner' values.
    while (count < leaves.size()) {
        uint256 h = leaves[count];
        bool matchh = count == branchpos;
        count++;
        int level;
        // For each of the lower bits in count that are 0, do 1 step. Each
        // corresponds to an inner value that existed before processing the
        // current leaf, and each needs a hash to combine it.
        for (level = 0; !(count & (((uint32_t)1) << level)); level++) {
            if (pbranch) {
                if (matchh) {
                    pbranch->push_back(inner[level]);
                } else if (matchlevel == level) {
                    pbranch->push_back(h);
                    matchh = true;
                }
            }
            mutated |= (inner[level] == h);
            MerkleHash(h, inner[level], h);
        }
        // Store the resulting hash at inner position level.
        inner[level] = h;
        if (matchh) {
            matchlevel = level;
        }
    }
    // Do a final 'sweep' over the rightmost branch of the tree to process
    // odd levels, and reduce everything to a single top value.
    // Level is the level (counted from the bottom) up to which we've sweeped.
    int level = 0;
    // As long as bit number level in count is zero, skip it. It means there
    // is nothing left at this level.
    while (!(count & (((uint32_t)1) << level))) {
        level++;
    }
    uint256 h = inner[level];
    bool matchh = matchlevel == level;
    while (count != (((uint32_t)1) << level)) {
        // If we reach this point, h is an inner value that is not the top.
        // We combine it with itself (Freicoin's special rule for odd levels in
        // the tree) to produce a higher level one.
        if (is_mutable && !is_stable && pbranch && matchh) {
            pbranch->push_back(h);
        }
        if (is_mutable) {
            MerkleHash(h, h, h);
        }
        // Increment count to the value it would have if two entries at this
        // level had existed.
        count += (((uint32_t)1) << level);
        level++;
        // And propagate the result upwards accordingly.
        while (!(count & (((uint32_t)1) << level))) {
            if (pbranch) {
                if (matchh) {
                    pbranch->push_back(inner[level]);
                } else if (matchlevel == level) {
                    pbranch->push_back(h);
                    matchh = true;
                }
            }
            MerkleHash(h, inner[level], h);
            level++;
        }
    }
    // Return result.
    if (pmutated) *pmutated = mutated;
    if (proot) *proot = h;
}

uint256 ComputeMerkleRoot(const std::vector<uint256>& leaves, bool* mutated) {
    uint256 hash;
    MerkleComputation(leaves, &hash, mutated, -1, nullptr, MERKLE_COMPUTATION_MUTABLE);
    return hash;
}

std::vector<uint256> ComputeMerkleBranch(const std::vector<uint256>& leaves, uint32_t position) {
    std::vector<uint256> ret;
    MerkleComputation(leaves, nullptr, nullptr, position, &ret, MERKLE_COMPUTATION_MUTABLE);
    return ret;
}

uint256 ComputeMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& vMerkleBranch, uint32_t nIndex) {
    uint256 hash = leaf;
    for (std::vector<uint256>::const_iterator it = vMerkleBranch.begin(); it != vMerkleBranch.end(); ++it) {
        if (nIndex & 1) {
            hash = Hash(BEGIN(*it), END(*it), BEGIN(hash), END(hash));
        } else {
            hash = Hash(BEGIN(hash), END(hash), BEGIN(*it), END(*it));
        }
        nIndex >>= 1;
    }
    return hash;
}

std::pair<std::vector<uint256>, std::pair<uint32_t, uint32_t> > ComputeStableMerkleBranch(const std::vector<uint256>& leaves, uint32_t pos) {
    std::vector<uint256> ret;
    MerkleComputation(leaves, nullptr, nullptr, pos, &ret, MERKLE_COMPUTATION_MUTABLE|MERKLE_COMPUTATION_STABLE);
    return {ret, ComputeMerklePathAndMask(ret.size(), pos)};
}

uint256 ComputeStableMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t path, uint32_t mask, bool *mutated) {
    if (mutated) {
        *mutated = false;
    }
    uint256 hash = leaf;
    for (std::vector<uint256>::const_iterator it = branch.begin(); it != branch.end(); ) {
        if (mask & 1) {
            hash = Hash(BEGIN(hash), END(hash), BEGIN(hash), END(hash));
        } else {
            if (path & 1) {
                hash = Hash(BEGIN(*it), END(*it), BEGIN(hash), END(hash));
            } else {
                hash = Hash(BEGIN(hash), END(hash), BEGIN(*it), END(*it));
            }
            ++it;
            path >>= 1;
        }
        mask >>= 1;
    }
    /* Perform any repeated hashes between the last given branch hash and the
     * next (missing) hash.  The particular use case for this is computing the
     * root of a subtree, such as the recomputing the block-final transaction
     * branch of the Merkle tree when the segwit commitment is updated.  In all
     * practical situations you want these final repeated hashes to be done,
     * since the result is the hash value which actually shows up in other
     * branches. */
    while (mask & 1) {
        hash = Hash(BEGIN(hash), END(hash), BEGIN(hash), END(hash));
        mask >>= 1;
    }
    if (mutated && (path || mask)) {
        *mutated = true;
    }
    return hash;
}

uint256 ComputeFastMerkleRoot(const std::vector<uint256>& leaves) {
    if (leaves.empty()) {
        return CHashWriter(SER_GETHASH, PROTOCOL_VERSION).GetHash();
    }
    uint256 hash;
    MerkleComputation(leaves, &hash, nullptr, -1, nullptr, MERKLE_COMPUTATION_FAST);
    return hash;
}

std::pair<uint32_t, uint32_t> ComputeMerklePathAndMask(uint32_t branchlen, uint32_t position)
{
    /* Calculate the largest possible size the branch vector can be.
     * This is one more than the zero-based index of the highest set
     * bit. */
    std::size_t max = 0;
    for (max = 32; max > 0; --max)
        if (position & ((uint32_t)1)<<(max-1))
            break;
    /* If the number of returned hashes in the branch vector is less
     * than the maximum allowed size, it must be because the branch
     * lies at least partially along the right-most path of an
     * unbalanced tree.
     *
     * We calculate the path by dropping the necessary number of
     * most-significant zero bits from the binary representation of
     * position. */
    uint32_t mask = 0;
    uint32_t path = position;
    while (max > branchlen) {
        /* Find the first clear/zero bit *below* the most significant
         * set bit.  We do this by starting with what we know to be
         * the most-significant set bit, and then working backwards
         * until we hit a zero bit. */
        int i;
        for (i = max-1; i >= 0; --i)
            if (!(path & ((uint32_t)1)<<i))
                break;
        /* It will never be the case that we don't find a zero bit as
         * in that situation MerkleComputation would have returned
         * more hashes.  However for the purpose helping code analysis
         * tools which can't make this inference, we detect failure
         * and return. */
        if (i < 0) // Should never happen
            return {};
        /* Set the i'th bit of the mask. */
        mask |= ((uint32_t)1) << i;
        /* Bit-fiddle to build two masks: one covering all the bits
         * above the i'th position, and one covering all bits below.
         * Apply both to path and shift the top bits down by one
         * position, effectively eliminating the i'th bit. */
        path = ((path & ~((((uint32_t)1)<<(i+1))-1))>>1) // e.g. 0b11111111111111111111111100000000
             |  (path &  ((((uint32_t)1)<< i   )-1));    //      0b00000000000000000000000001111111
        --max;                                           //                                |
    }                                                    //                        i = 7 --^
    return {path, mask};
}

std::pair<std::vector<uint256>, uint32_t> ComputeFastMerkleBranch(const std::vector<uint256>& leaves, uint32_t position) {
    std::vector<uint256> branch;
    MerkleComputation(leaves, nullptr, nullptr, position, &branch, MERKLE_COMPUTATION_FAST);
    return {branch, ComputeMerklePathAndMask(branch.size(), position).first};
}

uint256 ComputeFastMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t path, bool* invalid) {
    uint256 hash = leaf;
    for (const uint256& h : branch) {
        if (path & 1) {
            MerkleHash_Sha256Midstate(hash, h, hash);
        } else {
            MerkleHash_Sha256Midstate(hash, hash, h);
        }
        path >>= 1;
    }
    if (invalid) {
        *invalid = (path != 0);
    }
    return hash;
}

static uint256 calc_bits(const uint256& key, int begin, int end)
{
    assert(begin <= end);
    uint256 ret; // = 0
    for (int idx = begin; idx < end; ++idx) {
        unsigned char src = 255 - idx;
        unsigned char dst = end - idx - 1;
        if (key.begin()[31-(src/8)] & (1 << (src%8))) {
            ret.begin()[31-(dst/8)] |= 1 << (dst%8);
        }
    }
    ret.begin()[31-((end-begin)/8)] |= 1 << ((end-begin)%8);
    return ret;
}

static uint256 calc_remainder(const uint256& key, int used)
{
    assert(used <= 255);
    if (!used) {
        return key;
    }
    uint256 ret; // = 0
    for (int idx = 0; idx < (256-used); ++idx) {
        if (key.begin()[31-(idx/8)] & (1 << (idx%8))) {
            ret.begin()[31-(idx/8)] |= 1 << (idx%8);
        }
    }
    return ret;
}

uint256 ComputeMerkleMapRootFromBranch(const uint256& value, const std::vector<std::pair<unsigned char, uint256> >& branch, const uint256& key, bool* invalid)
{
    size_t total = 0;
    for (auto& skip : branch) {
        total += 1 + (size_t)skip.first;
    }

    if (total >= 256) {
        if (invalid) {
            *invalid = true;
        }
        return {};
    }

    if (invalid) {
        *invalid = false;
    }

    uint256 hash;
    MerkleHash_Sha256Midstate(hash, calc_remainder(key, total), value);
    for (auto& skip : branch) {
        --total;
        auto begin = total - skip.first;
        auto end = total;
        if (key.begin()[31-(end/8)] & (1 << (end%8))) {
            MerkleHash_Sha256Midstate(hash, skip.second, hash);
        } else {
            MerkleHash_Sha256Midstate(hash, hash, skip.second);
        }
        MerkleHash_Sha256Midstate(hash, calc_bits(key, begin, end), hash);
    }

    return hash;
}

uint256 BlockMerkleRoot(const CBlock& block, bool* mutated)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    for (size_t s = 0; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s].GetHash();
    }
    return ComputeMerkleRoot(leaves, mutated);
}

uint256 BlockWitnessMerkleRoot(const CBlock& block)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());

    // The coinbase's witness contains the witness nonce, which cannot be
    // included under the witness Merkle root.
    leaves.front() = block.vtx.front().GetHash();

    // The witness Merkle root is placed in the block-final
    // transaction, but it is a Merkle tree of all transactions,
    // including itself.  To avoid this impossibility, when computing
    // the witness Merkle root the commitment is set to zero.
    CDataStream tx(SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
    tx << block.vtx.back();

    // Check if there is a witness commitment:
    if (tx.size() >= (1 + 32 + 4 + 8) // <- 1 byte for witness path
        && tx[tx.size()-8-4] == 0x4b  //   32 bytes for merkle root
        && tx[tx.size()-8-3] == 0x4a  //    4 bytes for magic value
        && tx[tx.size()-8-2] == 0x49  //    4 bytes for nLockTime
        && tx[tx.size()-8-1] == 0x48) //    4 bytes for lock_height
    {                                 //      (end of transaction)
        // Set the witness commitment bytes to 0.
        std::fill_n(tx.end()-8-4-32-1, 33, 0);
    }
    // The block-final transaction never contains any witness data.
    CHash256()
        .Write((unsigned char*)&tx[0], tx.size())
        .Finalize(leaves.back().begin());

    for (size_t s = 1; s < block.vtx.size()-1; s++) {
        leaves[s] = block.vtx[s].GetWitnessHash();
    }

    return ComputeFastMerkleRoot(leaves);
}

std::vector<uint256> BlockMerkleBranch(const CBlock& block, uint32_t position)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    for (size_t s = 0; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s].GetHash();
    }
    return ComputeMerkleBranch(leaves, position);
}
