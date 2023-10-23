// Copyright (c) 2017-2021 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkleproof.h>
#include <hash.h>
#include <version.h>

extern uint256 MerkleHash_Sha256Midstate(const uint256& left, const uint256& right);

/*
 * The {SKIP, SKIP} entry is missing on purpose. Not only does this
 * make the number of possible states a nicely packable power of 2,
 * but excluding that fully prunable state means that any given fully
 * expanded tree and set of verify hashes has one and only one proof
 * encoding -- the serialized tree with all {SKIP, SKIP} nodes
 * recursively pruned.
 *
 * The ordering of these entries is also specially chosen: it allows
 * for lexigraphical ordering of proofs extracted from the same tree
 * to stand-in for lexigraphical ordering of the underlying elements
 * if interpreted as an ordered list.
 */
const std::array<MerkleLink, 8> MerkleNode::m_left_from_code {{
    MerkleLink::VERIFY,  MerkleLink::VERIFY,  MerkleLink::VERIFY,
    MerkleLink::DESCEND, MerkleLink::DESCEND, MerkleLink::DESCEND,
      /* No SKIP */      MerkleLink::SKIP,    MerkleLink::SKIP }};

const std::array<MerkleLink, 8> MerkleNode::m_right_from_code {{
    MerkleLink::SKIP, MerkleLink::VERIFY, MerkleLink::DESCEND,
    MerkleLink::SKIP, MerkleLink::VERIFY, MerkleLink::DESCEND,
      /* No SKIP */   MerkleLink::VERIFY, MerkleLink::DESCEND }};

MerkleNode::code_type MerkleNode::_encode(MerkleLink left, MerkleLink right)
{
    /* Returns the 3-bit code for a given combination of left and
     * right link values in an internal node. */
    code_type code = std::numeric_limits<code_type>::max();
    /* Write out a table of Code values to see why this works :) */
    switch (left)
    {
        case MerkleLink::DESCEND: code = 5; break;
        case MerkleLink::VERIFY:  code = 2; break;
        case MerkleLink::SKIP:    code = 7; break;
    }
    switch (right)
    {
        case MerkleLink::SKIP:    --code; // No break!
        case MerkleLink::VERIFY:  --code; break;
        case MerkleLink::DESCEND:         break;
    }
    return code;
}

MerkleNode::code_type MerkleNodeReference::GetCode() const
{
    /* Belts and suspenders: m_offset should never be anything outside
     * the range [0, 7], so the assignment to max, which is necessary
     * to avoid compiler whining, should be undone by the switch that
     * follows.  But just in case we'll favor failing in a way that is
     * maximally likely to be cause trouble when the code value is
     * later used. */
    MerkleNode::code_type code = std::numeric_limits<MerkleNode::code_type>::max();
    switch (m_offset)
    {
        /* Use the diagram in the class definition to work out that
         * these magic constant values are correct. */
        case 0: code =   m_base[0] >> 5;       break;
        case 1: code =   m_base[0] >> 2;       break;
        case 2: code =  (m_base[0] << 1)
                     | ((m_base[1] >> 7) & 1); break;
        case 3: code =   m_base[1] >> 4;       break;
        case 4: code =   m_base[1] >> 1;       break;
        case 5: code =  (m_base[1] << 2)
                     | ((m_base[2] >> 6) & 3); break;
        case 6: code =   m_base[2] >> 3;       break;
        case 7: code =   m_base[2];            break;
        /* offset should never be outside the range of [0, 7], so we
         * should have been handled by one of the prior conditions and
         * this code is unreachable.  But as a matter of defensive
         * coding we throw a runtime error rather than return a
         * nonsense value. */
        default:
            throw std::runtime_error("MerkleNode::m_offset out of bounds");
    }
    return code & 7;
}

MerkleNodeReference& MerkleNodeReference::SetCode(MerkleNode::code_type code)
{
    switch (m_offset)
    {
        /* Again, check the diagram in the class definition to see
         * where these magic constant shift and mask values arise
         * from. */
        case 0: m_base[0] = (m_base[0] & 0x1f) |  (code      << 5); break;
        case 1: m_base[0] = (m_base[0] & 0xe3) |  (code      << 2); break;
        case 2: m_base[0] = (m_base[0] & 0xfc) |  (code      >> 1);
                m_base[1] = (m_base[1] & 0x7f) | ((code & 1) << 7); break;
        case 3: m_base[1] = (m_base[1] & 0x8f) |  (code      << 4); break;
        case 4: m_base[1] = (m_base[1] & 0xf1) |  (code      << 1); break;
        case 5: m_base[1] = (m_base[1] & 0xfe) |  (code      >> 2);
                m_base[2] = (m_base[2] & 0x3f) | ((code & 3) << 6); break;
        case 6: m_base[2] = (m_base[2] & 0xc7) |  (code      << 3); break;
        case 7: m_base[2] = (m_base[2] & 0xf8) |   code;            break;
        /* offset should never be outside the range of [0, 7], so we
         * should have been handled by one of the prior conditions and
         * this code is unreachable.  But as a matter of defensive
         * coding we throw a runtime error rather than try to guess
         * why we got here. */
        default:
            throw std::runtime_error("MerkleNode::m_offset out of bounds");
    }
    return *this;
}

void MerkleNodeIteratorBase::_incr()
{
    if (m_ref.m_offset++ == 7) {
        m_ref.m_offset = 0;
        m_ref.m_base += 3;
    }
}

void MerkleNodeIteratorBase::_decr()
{
    if (m_ref.m_offset-- == 0) {
        m_ref.m_offset = 7;
        m_ref.m_base -= 3;
    }
}

void MerkleNodeIteratorBase::_seek(MerkleNodeIteratorBase::difference_type distance)
{
    difference_type bits = distance + m_ref.m_offset;
    m_ref.m_base += 3 * (bits / 8);
    bits = bits % 8;
    if (bits < 0) {
        bits += 8;
        m_ref.m_base -= 3;
    }
    m_ref.m_offset = static_cast<MerkleNodeReference::offset_type>(bits);
}

MerkleNodeIteratorBase::difference_type MerkleNodeIteratorBase::operator-(const MerkleNodeIteratorBase& other) const
{
    /* Compare with the version of _seek implemented above. The
     * following property should hold true:
     *
     *   A._seek(B-A) == B
     *
     * That is to say, the difference between two iterators is the
     * value which needs to be passed to _seek() to move from one to
     * the other. */
    return 8 * (m_ref.m_base - other.m_ref.m_base) / 3 + m_ref.m_offset - other.m_ref.m_offset;
}

void MerkleBranch::clear() noexcept
{
    m_branch.clear();
    m_vpath.clear();
}

void swap(MerkleBranch& lhs, MerkleBranch& rhs)
{
    using std::swap;
    swap(lhs.m_branch, rhs.m_branch);
    swap(lhs.m_vpath, rhs.m_vpath);
}

uint32_t MerkleBranch::GetPath() const
{
    uint32_t ret = 0;
    uint32_t mask = 1;
    int pos = 0;
    for (; pos < m_vpath.size() && pos < 32; ++pos) {
        ret |= static_cast<uint32_t>(m_vpath[pos]) << pos;
    }
    // We only throw an error if we are required to set a bit
    // which is too high to be contained in type uint32_t.  Note
    // that this means a path which is more than 32 bits in length
    // could be accepted, so long as the high-order bits are zero.
    for (; pos < m_vpath.size(); ++pos) {
        if (m_vpath[pos]) {
            throw std::runtime_error("MerkleBranch::GetPath : vpath does not fit within standard integer");
        }
    }
    return ret;
}

std::vector<unsigned char> MerkleBranch::getvch() const
{
    std::vector<unsigned char> ret((m_vpath.size() + 7) / 8, 0);
    for (std::size_t pos = 0; pos < m_vpath.size(); ++pos) {
        ret[pos/8] |= static_cast<unsigned char>(m_vpath[pos]) << (pos % 8);
    }
    while (!ret.empty() && ret.back() == 0) {
        ret.pop_back();
    }
    for (auto skip : m_branch) {
        ret.insert(ret.end(), skip.begin(), skip.end());
    }
    return ret;
}

MerkleBranch& MerkleBranch::setvch(const std::vector<unsigned char>& data)
{
    using std::swap;
    if (data.size() > 1028) { // 1028 = 32*32 + (32/8)
        throw std::runtime_error("MerkleBranch::setvch : byte vector is too large to contain branch of 32 hashes or less");
    }
    const int bytes_in_path = data.size() % 32;
    const int max_bytes_in_path = ((data.size() / 32) + 7) / 8;
    if (bytes_in_path > max_bytes_in_path) {
        throw std::runtime_error("MerkleBranch::setvch : residual bytes for path is greater than 4 (> 32 bits)");
    }
    if (bytes_in_path && (data[bytes_in_path-1] == 0)) {
        throw std::runtime_error("MerkleBranch::setvch : path is not minimally encoded");
    }
    m_vpath.clear();
    m_vpath.resize(data.size() / 32, false);
    for (int i = 0; i < bytes_in_path; ++i) {
        for (int j = 0; j < 8; ++j) {
            bool bit = data[i] & (static_cast<unsigned char>(1) << j);
            if ((i*8 + j) < m_vpath.size()) {
                m_vpath[i*8+j] = bit;
            } else if (bit) {
                throw std::runtime_error("MerkleBranch::setvch : dirty bit set in path");
            }
        }
    }
    m_branch.clear();
    m_branch.reserve(data.size() / 32);
    for (auto ptr = data.begin() + bytes_in_path; ptr != data.end(); ptr += 32) {
        m_branch.emplace_back(std::vector<unsigned char>(ptr, ptr + 32));
    }
    return *this;
}

void MerkleProof::clear() noexcept
{
    m_path.clear();
    m_skip.clear();
}

void swap(MerkleProof& lhs, MerkleProof& rhs)
{
    using std::swap;
    swap(lhs.m_path, rhs.m_path);
    swap(lhs.m_skip, rhs.m_skip);
}

MerkleTree::MerkleTree(const uint256& hash, bool verify)
{
    /* A tree consisting of a single hash, whether VERIFY or SKIP, is
     * represented with no nodes. */
    if (verify) {
        m_verify.emplace_back(hash);
    } else {
        m_proof.m_skip.emplace_back(hash);
    }
}

MerkleTree::MerkleTree(const uint256& leaf, const MerkleBranch& branch)
    : m_verify(1, leaf)
{
    assert(branch.m_vpath.size() == branch.m_branch.size());

    /* If the branch proof is empty, then we're talking about a single
     * VERIFY hash.  The hash has already been placed in m_verify. */
    if (branch.m_vpath.empty())
        return;

    /* There is going to be one internal node for each SKIP hash, and
     * the branch proof consists entirely of SKIP hashes. */
    m_proof.m_path.reserve(branch.m_vpath.size());
    m_proof.m_skip.reserve(branch.m_branch.size());

    /* The branch proof consists of SKIP hashes in order of decreasing
     * depth, from the leaf's level to the root node of the tree.  The
     * MerkleProof on the other hand stores the hashes in order of
     * left-to-right, in-order traversal.
     *
     * This means that all "left" SKIP hashes will come before all of
     * the "right" SKIP hashes, and the "left" hashes are stored in
     * increasing order of depth while the "right" hashes are in
     * decreasing depth order.
     *
     * To reorder the hashes, we scan the branch proof twice, in
     * different directions.  While we're at it we also build the node
     * representation of the tree. */

    /* Scan from the top of the tree to the leaf, adding "left"-side
     * SKIP hashes in the opposite order of the branch proof, and
     * building the node representation. */
    {
        auto side = branch.m_vpath.rbegin();
        auto hash = branch.m_branch.rbegin();
        for (; side != branch.m_vpath.rend(); ++side, ++hash) {
            if (*side == false) {
                m_proof.m_path.emplace_back(
                    MerkleLink::DESCEND,
                    MerkleLink::SKIP);
            } else {
                m_proof.m_path.emplace_back(
                    MerkleLink::SKIP,
                    MerkleLink::DESCEND);
                m_proof.m_skip.push_back(*hash);
            }
        }
    }

    /* Scan from the bottom to the top, adding "right"-side SKIP
     * hashes in the same order as the branch proof. */
    {
        auto side = branch.m_vpath.begin();
        auto hash = branch.m_branch.begin();
        for (; side != branch.m_vpath.end(); ++side, ++hash) {
            if (*side == false) {
                m_proof.m_skip.push_back(*hash);
            }
        }
    }

    /* The DESCEND branch of the final node needs to get changed to a
     * VERIFY hash, the leaf. */
    if (branch.m_vpath.front()) {
        m_proof.m_path.back().SetRight(MerkleLink::VERIFY);
    } else {
        m_proof.m_path.back().SetLeft(MerkleLink::VERIFY);
    }
}

MerkleTree::MerkleTree(const MerkleTree& left, const MerkleTree& right)
{
    /* Handle the special case of either the left or right being
     * empty, which is idempotent. */
    if (left == MerkleTree()) {
        m_proof = right.m_proof;
        m_verify = right.m_verify;
        return;
    }

    else if (right == MerkleTree()) {
        m_proof = left.m_proof;
        m_verify = left.m_verify;
        return;
    }

    /* Handle the special case of both left and right being fully
     * pruned, which also results in a fully pruned super-tree. */
    if (left.m_proof.m_path.empty() && left.m_proof.m_skip.size()==1 && left.m_verify.empty() &&
        right.m_proof.m_path.empty() && right.m_proof.m_skip.size()==1 && right.m_verify.empty())
    {
        m_proof.m_skip.resize(1);
        m_proof.m_skip[0] = MerkleHash_Sha256Midstate(left.m_proof.m_skip[0], right.m_proof.m_skip[0]);
        return;
    }

    /* We assume a well-formed, non-empty MerkleTree for both passed
     * in subtrees, in which if there are no internal nodes than
     * either m_skip XOR m_verify must have a single hash.  Otherwise
     * the result of what follows will be an invalid MerkleTree. */
    m_proof.m_path.emplace_back(MerkleLink::DESCEND, MerkleLink::DESCEND);

    if (left.m_proof.m_path.empty())
        m_proof.m_path.front().SetLeft(left.m_proof.m_skip.empty()? MerkleLink::VERIFY: MerkleLink::SKIP);
    m_proof.m_path.insert(m_proof.m_path.end(), left.m_proof.m_path.begin(), left.m_proof.m_path.end());
    m_proof.m_skip.insert(m_proof.m_skip.end(), left.m_proof.m_skip.begin(), left.m_proof.m_skip.end());
    m_verify.insert(m_verify.end(), left.m_verify.begin(), left.m_verify.end());

    if (right.m_proof.m_path.empty())
        m_proof.m_path.front().SetRight(right.m_proof.m_skip.empty()? MerkleLink::VERIFY: MerkleLink::SKIP);
    m_proof.m_path.insert(m_proof.m_path.end(), right.m_proof.m_path.begin(), right.m_proof.m_path.end());
    m_proof.m_skip.insert(m_proof.m_skip.end(), right.m_proof.m_skip.begin(), right.m_proof.m_skip.end());
    m_verify.insert(m_verify.end(), right.m_verify.begin(), right.m_verify.end());
}

void MerkleTree::clear() noexcept
{
    m_proof.clear();
    m_verify.clear();
}

void swap(MerkleTree& lhs, MerkleTree& rhs)
{
    using std::swap;
    swap(lhs.m_proof, rhs.m_proof);
    swap(lhs.m_verify, rhs.m_verify);
}

uint256 MerkleTree::GetHash(bool* invalid, std::vector<MerkleBranch>* branches) const
{
    using std::swap;

    /* As a special case, an empty proof with no verify hashes results
     * in the unsalted hash of empty string.  Although this requires
     * extra work in this implementation to support, it provides
     * continuous semantics to the meaning of the MERKLEBLOCKVERIFY
     * opcode, which might potentially reduce the number of code paths
     * in some scripts. */
    if (m_proof.m_path.empty() && m_verify.empty() && m_proof.m_skip.empty()) {
        if (invalid) {
            *invalid = false;
        }
        if (branches) {
            branches->clear();
        }
        return CHashWriter(SER_GETHASH, PROTOCOL_VERSION).GetHash();
    }

    /* Except for the special case of a 0-node, 0-verify, 0-skip tree,
     * it is always the case (for any binary tree), that the number of
     * leaf nodes (verify + skip) is one more than the number of
     * internal nodes in the tree. */
    if ((m_verify.size() + m_proof.m_skip.size()) != (m_proof.m_path.size() + 1)) {
        if (invalid) {
            *invalid = true;
        }
        return uint256();
    }

    /* If there are NO nodes in the tree, then this is the degenerate
     * case of a single hash, in either the verify or skip set. */
    if (m_proof.m_path.empty()) {
        if (invalid) {
            *invalid = false;
        }
        if (branches) {
            branches->clear();
            // If it is a verify hash, then the caller expects a
            // proof.
            if (!m_verify.empty()) {
                branches->emplace_back();
            }
        }
        return m_verify.empty() ? m_proof.m_skip[0]
                                : m_verify[0];
    }

    std::vector<std::pair<bool, uint256> > stack(2);
    std::size_t verify_pos = 0;
    std::size_t skip_pos = 0;

    std::vector<MerkleBranch> proofs(m_verify.size());
    std::vector<std::size_t> extra_depths(m_verify.size(), 0);
    std::vector<bool> vpath;

    auto visitor = [this, &stack, &verify_pos, &skip_pos, &proofs, &extra_depths, &vpath](std::size_t depth, MerkleLink value, bool side) -> bool
    {
        const uint256* new_hash = nullptr;
        switch (value) {
            case MerkleLink::DESCEND:
                for (auto pos = 0; pos < verify_pos; ++pos) {
                    ++extra_depths[pos];
                }
                vpath.push_back(side);
                stack.emplace_back();
                return false;

            case MerkleLink::VERIFY:
                if (verify_pos == m_verify.size()) // detect read past end of verify hashes list
                    return true;
                // Include the last branch in the path for this hash:
                proofs[verify_pos].m_vpath.push_back(side);
                proofs[verify_pos].m_vpath.insert(
                    proofs[verify_pos].m_vpath.end(),
                    vpath.rbegin(), vpath.rend());
                new_hash = &m_verify[verify_pos++];
                break;

            case MerkleLink::SKIP:
                if (skip_pos == m_proof.m_skip.size()) // detect read past end of skip hashes list
                    return true;
                new_hash = &m_proof.m_skip[skip_pos++];
                break;
        }

        uint256 tmp;
        while (stack.back().first) {
            for (auto pos = 0; pos < verify_pos; ++pos) {
                if (extra_depths[pos]) {
                    --extra_depths[pos];
                } else {
                    // Note the off-by-1 "error" in the expression
                    // below.  The root of the tree is at depth 0, so
                    // the first links are at depth 1. Our stack is
                    // zero based, of course, so `depth` needs to be
                    // offset by 1, which it implicitly is in the
                    // subtraction.
                    if (proofs[pos].m_vpath[proofs[pos].m_vpath.size()-depth]) {
                        proofs[pos].m_branch.push_back(stack.back().second);
                    } else {
                        proofs[pos].m_branch.push_back(*new_hash);
                    }
                }
            }
            vpath.pop_back();
            tmp = MerkleHash_Sha256Midstate(stack.back().second, *new_hash);
            new_hash = &tmp;
            stack.pop_back();
            --depth;
        }

        stack.back().first = true;
        stack.back().second = *new_hash;
        return false;
    };

    auto res = depth_first_traverse(m_proof.m_path.begin(), m_proof.m_path.end(), visitor);

    if (res.first != m_proof.m_path.end() // m_proof.m_path has "extra" Nodes (after end of tree)
        || res.second                     // m_proof.m_path has insufficient Nodes (tree not finished)
        || stack.size() != 1              // expected one root hash...
        || !stack.back().first)           // ...and an actual hash, not a placeholder
    {
        if (invalid) {
            *invalid = true;
        }
        return uint256();
    }

    bool failed = (verify_pos != m_verify.size() // did not use all verify hashes
                  || skip_pos != m_proof.m_skip.size()); // did not use all skip hashes
    if (invalid) {
        *invalid = failed;
    }
    if (failed) {
        return uint256();
    }

    if (branches) {
        std::swap(*branches, proofs);
    }
    return stack.back().second;
}

// End of File
