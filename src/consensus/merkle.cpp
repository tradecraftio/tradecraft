// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <hash.h>

/*     WARNING! If you're reading this because you're learning about crypto
       and/or designing a new system that will use merkle trees, keep in mind
       that the following merkle tree algorithm has a serious flaw related to
       duplicate txids, resulting in a vulnerability (CVE-2012-2459).

       The reason is that if the number of hashes in the list at a given level
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

uint256 MerkleHash_Hash256(const uint256& left, const uint256& right) {
    return Hash(left, right);
}

/* Calculated by using standard FIPS-180 SHA-256 to produce the digest of the
 * empty string / zero-length byte array, then feeding the resulting digest into
 * SHA-256 twice in order to fill a block and trigger a compression round.  The
 * midstate is then extracted and used as our hash value.  Expressed with the
 * pure-Python sha256 package, this would be something like the following:
 *
 *     import struct
 *     from sha256 import SHA256
 *     iv = b''.join([struct.pack(">I", i) for i in
 *                    SHA256(SHA256(b"").digest() +
 *                           SHA256(b"").digest()).state])
 */
static unsigned char _MidstateIV[32] =
    { 0x1e, 0x4e, 0x0f, 0x95, 0x5a, 0x4b, 0xc8, 0x1c,
      0x08, 0xc8, 0xaf, 0x1c, 0x94, 0xf3, 0x4b, 0x9d,
      0x0a, 0xf2, 0xf4, 0x50, 0xdc, 0x24, 0xa3, 0xbc,
      0xef, 0x98, 0x31, 0x8f, 0xaf, 0x5e, 0x25, 0x06 };
uint256 MerkleHash_Sha256Midstate(const uint256& left, const uint256& right) {
    uint256 parent;
    CSHA256(_MidstateIV).Write(left.begin(), 32).Write(right.begin(), 32).Midstate(parent.begin(), nullptr, nullptr);
    return parent;
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
        for (level = 0; !(count & ((uint32_t{1}) << level)); level++) {
            if (pbranch) {
                if (matchh) {
                    pbranch->push_back(inner[level]);
                } else if (matchlevel == level) {
                    pbranch->push_back(h);
                    matchh = true;
                }
            }
            mutated |= (inner[level] == h);
            h = MerkleHash(inner[level], h);
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
    while (!(count & ((uint32_t{1}) << level))) {
        level++;
    }
    uint256 h = inner[level];
    bool matchh = matchlevel == level;
    while (count != ((uint32_t{1}) << level)) {
        // If we reach this point, h is an inner value that is not the top.
        // We combine it with itself (Bitcoin's special rule for odd levels in
        // the tree) to produce a higher level one.
        if (is_mutable && pbranch && matchh) {
            pbranch->push_back(h);
        }
        if (is_mutable) {
            h = MerkleHash(h, h);
        }
        // Increment count to the value it would have if two entries at this
        // level had existed.
        count += ((uint32_t{1}) << level);
        level++;
        // And propagate the result upwards accordingly.
        while (!(count & ((uint32_t{1}) << level))) {
            if (pbranch) {
                if (matchh) {
                    pbranch->push_back(inner[level]);
                } else if (matchlevel == level) {
                    pbranch->push_back(h);
                    matchh = true;
                }
            }
            h = MerkleHash(inner[level], h);
            level++;
        }
    }
    // Return result.
    if (pmutated) *pmutated = mutated;
    if (proot) *proot = h;
}

uint256 ComputeMerkleRoot(std::vector<uint256> hashes, bool* mutated) {
    bool mutation = false;
    while (hashes.size() > 1) {
        if (mutated) {
            for (size_t pos = 0; pos + 1 < hashes.size(); pos += 2) {
                if (hashes[pos] == hashes[pos + 1]) mutation = true;
            }
        }
        if (hashes.size() & 1) {
            hashes.push_back(hashes.back());
        }
        SHA256D64(hashes[0].begin(), hashes[0].begin(), hashes.size() / 2);
        hashes.resize(hashes.size() / 2);
    }
    if (mutated) *mutated = mutation;
    if (hashes.size() == 0) return uint256();
    return hashes[0];
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
            hash = Hash(*it, hash);
        } else {
            hash = Hash(hash, *it);
        }
        nIndex >>= 1;
    }
    return hash;
}

uint256 ComputeFastMerkleRoot(const std::vector<uint256>& leaves) {
    if (leaves.empty()) {
        return HashWriter{}.GetHash();
    }
    uint256 hash;
    MerkleComputation(leaves, &hash, nullptr, -1, nullptr, MERKLE_COMPUTATION_FAST);
    return hash;
}

std::pair<std::vector<uint256>, uint32_t> ComputeFastMerkleBranch(const std::vector<uint256>& leaves, uint32_t position) {
    std::vector<uint256> branch;
    MerkleComputation(leaves, nullptr, nullptr, position, &branch, MERKLE_COMPUTATION_FAST);
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
    uint32_t path = position;
    while (max > branch.size()) {
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
        /* Bit-fiddle to build two masks: one covering all the bits
         * above the i'th position, and one covering all bits below.
         * Apply both to path and shift the top bits down by one
         * position, effectively eliminating the i'th bit. */
        path = ((path & ~((((uint32_t)1)<<(i+1))-1))>>1) // e.g. 0b11111111111111111111111100000000
             |  (path &  ((((uint32_t)1)<< i   )-1));    //      0b00000000000000000000000001111111
        --max;                                           //                                |
    }                                                    //                        i = 7 --^
    return {branch, path};
}

uint256 ComputeFastMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t path, bool* invalid) {
    uint256 hash = leaf;
    for (const uint256& h : branch) {
        if (path & 1) {
            hash = MerkleHash_Sha256Midstate(h, hash);
        } else {
            hash = MerkleHash_Sha256Midstate(hash, h);
        }
        path >>= 1;
    }
    if (invalid) {
        *invalid = (path != 0);
    }
    return hash;
}

uint256 BlockMerkleRoot(const CBlock& block, bool* mutated)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    for (size_t s = 0; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetHash();
    }
    return ComputeMerkleRoot(std::move(leaves), mutated);
}

uint256 BlockWitnessMerkleRoot(const CBlock& block, bool* mutated)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    leaves[0].SetNull(); // The witness hash of the coinbase is 0.
    for (size_t s = 1; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetWitnessHash();
    }
    return ComputeMerkleRoot(std::move(leaves), mutated);
}

std::vector<uint256> BlockMerkleBranch(const CBlock& block, uint32_t position)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    for (size_t s = 0; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetHash();
    }
    return ComputeMerkleBranch(leaves, position);
}
