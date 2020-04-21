// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <consensus/merkleproof.h>
#include <test/util/setup_common.h>

#include <streams.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>
#include <deque>

BOOST_FIXTURE_TEST_SUITE(merkle_tests, TestingSetup)

// Older version of the merkle root computation code, for comparison.
static uint256 BlockBuildMerkleTree(const CBlock& block, bool* fMutated, std::vector<uint256>& vMerkleTree)
{
    vMerkleTree.clear();
    vMerkleTree.reserve(block.vtx.size() * 2 + 16); // Safe upper bound for the number of total nodes.
    for (std::vector<CTransactionRef>::const_iterator it(block.vtx.begin()); it != block.vtx.end(); ++it)
        vMerkleTree.push_back((*it)->GetHash());
    int j = 0;
    bool mutated = false;
    for (int nSize = block.vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            if (i2 == i + 1 && i2 + 1 == nSize && vMerkleTree[j+i] == vMerkleTree[j+i2]) {
                // Two identical hashes at the end of the list at a particular level.
                mutated = true;
            }
            vMerkleTree.push_back(Hash(vMerkleTree[j+i], vMerkleTree[j+i2]));
        }
        j += nSize;
    }
    if (fMutated) {
        *fMutated = mutated;
    }
    return (vMerkleTree.empty() ? uint256() : vMerkleTree.back());
}

// Older version of the merkle branch computation code, for comparison.
static std::vector<uint256> BlockGetMerkleBranch(const CBlock& block, const std::vector<uint256>& vMerkleTree, int nIndex)
{
    std::vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = block.vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        int i = std::min(nIndex^1, nSize-1);
        vMerkleBranch.push_back(vMerkleTree[j+i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

static inline int ctz(uint32_t i) {
    if (i == 0) return 0;
    int j = 0;
    while (!(i & 1)) {
        j++;
        i >>= 1;
    }
    return j;
}

BOOST_AUTO_TEST_CASE(merkle_test)
{
    for (int i = 0; i < 32; i++) {
        // Try 32 block sizes: all sizes from 0 to 16 inclusive, and then 15 random sizes.
        int ntx = (i <= 16) ? i : 17 + (InsecureRandRange(4000));
        // Try up to 3 mutations.
        for (int mutate = 0; mutate <= 3; mutate++) {
            int duplicate1 = mutate >= 1 ? 1 << ctz(ntx) : 0; // The last how many transactions to duplicate first.
            if (duplicate1 >= ntx) break; // Duplication of the entire tree results in a different root (it adds a level).
            int ntx1 = ntx + duplicate1; // The resulting number of transactions after the first duplication.
            int duplicate2 = mutate >= 2 ? 1 << ctz(ntx1) : 0; // Likewise for the second mutation.
            if (duplicate2 >= ntx1) break;
            int ntx2 = ntx1 + duplicate2;
            int duplicate3 = mutate >= 3 ? 1 << ctz(ntx2) : 0; // And for the third mutation.
            if (duplicate3 >= ntx2) break;
            int ntx3 = ntx2 + duplicate3;
            // Build a block with ntx different transactions.
            CBlock block;
            block.vtx.resize(ntx);
            for (int j = 0; j < ntx; j++) {
                CMutableTransaction mtx;
                mtx.nLockTime = j;
                block.vtx[j] = MakeTransactionRef(std::move(mtx));
            }
            // Compute the root of the block before mutating it.
            bool unmutatedMutated = false;
            uint256 unmutatedRoot = BlockMerkleRoot(block, &unmutatedMutated);
            BOOST_CHECK(unmutatedMutated == false);
            // Optionally mutate by duplicating the last transactions, resulting in the same merkle root.
            block.vtx.resize(ntx3);
            for (int j = 0; j < duplicate1; j++) {
                block.vtx[ntx + j] = block.vtx[ntx + j - duplicate1];
            }
            for (int j = 0; j < duplicate2; j++) {
                block.vtx[ntx1 + j] = block.vtx[ntx1 + j - duplicate2];
            }
            for (int j = 0; j < duplicate3; j++) {
                block.vtx[ntx2 + j] = block.vtx[ntx2 + j - duplicate3];
            }
            // Compute the merkle root and merkle tree using the old mechanism.
            bool oldMutated = false;
            std::vector<uint256> merkleTree;
            uint256 oldRoot = BlockBuildMerkleTree(block, &oldMutated, merkleTree);
            // Compute the merkle root using the new mechanism.
            bool newMutated = false;
            uint256 newRoot = BlockMerkleRoot(block, &newMutated);
            BOOST_CHECK(oldRoot == newRoot);
            BOOST_CHECK(newRoot == unmutatedRoot);
            BOOST_CHECK((newRoot == uint256()) == (ntx == 0));
            BOOST_CHECK(oldMutated == newMutated);
            BOOST_CHECK(newMutated == !!mutate);
            // If no mutation was done (once for every ntx value), try up to 16 branches.
            if (mutate == 0) {
                for (int loop = 0; loop < std::min(ntx, 16); loop++) {
                    // If ntx <= 16, try all branches. Otherwise, try 16 random ones.
                    int mtx = loop;
                    if (ntx > 16) {
                        mtx = InsecureRandRange(ntx);
                    }
                    std::vector<uint256> newBranch = BlockMerkleBranch(block, mtx);
                    std::vector<uint256> oldBranch = BlockGetMerkleBranch(block, merkleTree, mtx);
                    BOOST_CHECK(oldBranch == newBranch);
                    BOOST_CHECK(ComputeMerkleRootFromBranch(block.vtx[mtx]->GetHash(), newBranch, mtx) == oldRoot);
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE(merkle_test_empty_block)
{
    bool mutated = false;
    CBlock block;
    uint256 root = BlockMerkleRoot(block, &mutated);

    BOOST_CHECK_EQUAL(root.IsNull(), true);
    BOOST_CHECK_EQUAL(mutated, false);
}

BOOST_AUTO_TEST_CASE(merkle_test_oneTx_block)
{
    bool mutated = false;
    CBlock block;

    block.vtx.resize(1);
    CMutableTransaction mtx;
    mtx.nLockTime = 0;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    uint256 root = BlockMerkleRoot(block, &mutated);
    BOOST_CHECK_EQUAL(root, block.vtx[0]->GetHash());
    BOOST_CHECK_EQUAL(mutated, false);
}

BOOST_AUTO_TEST_CASE(merkle_test_OddTxWithRepeatedLastTx_block)
{
    bool mutated;
    CBlock block, blockWithRepeatedLastTx;

    block.vtx.resize(3);

    for (std::size_t pos = 0; pos < block.vtx.size(); pos++) {
        CMutableTransaction mtx;
        mtx.nLockTime = pos;
        block.vtx[pos] = MakeTransactionRef(std::move(mtx));
    }

    blockWithRepeatedLastTx = block;
    blockWithRepeatedLastTx.vtx.push_back(blockWithRepeatedLastTx.vtx.back());

    uint256 rootofBlock = BlockMerkleRoot(block, &mutated);
    BOOST_CHECK_EQUAL(mutated, false);

    uint256 rootofBlockWithRepeatedLastTx = BlockMerkleRoot(blockWithRepeatedLastTx, &mutated);
    BOOST_CHECK_EQUAL(rootofBlock, rootofBlockWithRepeatedLastTx);
    BOOST_CHECK_EQUAL(mutated, true);
}

BOOST_AUTO_TEST_CASE(merkle_test_LeftSubtreeRightSubtree)
{
    CBlock block, leftSubtreeBlock, rightSubtreeBlock;

    block.vtx.resize(4);
    std::size_t pos;
    for (pos = 0; pos < block.vtx.size(); pos++) {
        CMutableTransaction mtx;
        mtx.nLockTime = pos;
        block.vtx[pos] = MakeTransactionRef(std::move(mtx));
    }

    for (pos = 0; pos < block.vtx.size() / 2; pos++)
        leftSubtreeBlock.vtx.push_back(block.vtx[pos]);

    for (pos = block.vtx.size() / 2; pos < block.vtx.size(); pos++)
        rightSubtreeBlock.vtx.push_back(block.vtx[pos]);

    uint256 root = BlockMerkleRoot(block);
    uint256 rootOfLeftSubtree = BlockMerkleRoot(leftSubtreeBlock);
    uint256 rootOfRightSubtree = BlockMerkleRoot(rightSubtreeBlock);
    std::vector<uint256> leftRight;
    leftRight.push_back(rootOfLeftSubtree);
    leftRight.push_back(rootOfRightSubtree);
    uint256 rootOfLR = ComputeMerkleRoot(leftRight);

    BOOST_CHECK_EQUAL(root, rootOfLR);
}

BOOST_AUTO_TEST_CASE(merkle_test_BlockWitness)
{
    CBlock block;

    block.vtx.resize(2);
    for (std::size_t pos = 0; pos < block.vtx.size(); pos++) {
        CMutableTransaction mtx;
        mtx.nLockTime = pos;
        block.vtx[pos] = MakeTransactionRef(std::move(mtx));
    }

    uint256 blockWitness = BlockWitnessMerkleRoot(block);

    std::vector<uint256> hashes;
    hashes.resize(block.vtx.size());
    hashes[0].SetNull();
    hashes[1] = block.vtx[1]->GetHash();

    uint256 merkleRootofHashes = ComputeMerkleRoot(hashes);

    BOOST_CHECK_EQUAL(merkleRootofHashes, blockWitness);
}

BOOST_AUTO_TEST_CASE(merkle_stable_branch)
{
    using std::swap;

    std::string alphabet("abcdefghijklmnopqrstuv");
    BOOST_CHECK_EQUAL(alphabet.length(), 22); // last index == 0b10101

    uint256 hashZ;
    CHash256().Write({(const unsigned char*)"z", 1}).Finalize(hashZ);
    BOOST_CHECK(hashZ == uint256S("ca23f71f669346e53eb7679749b368c9ec09109b798ba542487224b79cd47cc2"));

    std::vector<uint256> leaves;
    for (auto c : alphabet) {
        uint256 hash;
        CHash256().Write({(unsigned char*)&c, 1}).Finalize(hash);
        leaves.push_back(hash);
    }
    BOOST_CHECK_EQUAL(leaves.size(), 22);
    BOOST_CHECK(leaves[0] == uint256S("d8f244c159278ea8cfffcbe1c463edef33d92d11d36ac3c62efd3eb7ff3a5dbf")); // just check the first hash, of 'a'

    for (uint32_t i = 0; i < leaves.size(); ++i) {
        std::vector<uint256> old_branch = ComputeMerkleBranch(leaves, i);
        auto res = ComputeStableMerkleBranch(leaves, i);
        std::vector<uint256>& new_branch = res.first;
        BOOST_CHECK_EQUAL(res.second.first, ComputeMerklePathAndMask(new_branch.size(), i).first);
        BOOST_CHECK_EQUAL(res.second.second, ComputeMerklePathAndMask(new_branch.size(), i).second);

        // Both branches should generate the same Merkle root.
        auto root = ComputeMerkleRoot(leaves, nullptr);
        BOOST_CHECK(root == ComputeMerkleRootFromBranch(leaves[i], old_branch, i));
        auto pathmask = ComputeMerklePathAndMask(new_branch.size(), i);
        BOOST_CHECK(root == ComputeStableMerkleRootFromBranch(leaves[i], new_branch, pathmask.first, pathmask.second, nullptr));

        if (i < 16) {
            // The first 16 branches are <0b100000, and therefore go down the
            // left-hand side of the tree and have no duplicated hashes.  The
            // results should therefore be identical with the old API.
            BOOST_CHECK(old_branch == new_branch);

            // Try replacing the leaf with hashZ
            swap(leaves[i], hashZ);
            root = ComputeMerkleRoot(leaves, nullptr);
            BOOST_CHECK(root == ComputeMerkleRootFromBranch(leaves[i], old_branch, i));
            auto pathmask = ComputeMerklePathAndMask(new_branch.size(), i);
            BOOST_CHECK(root == ComputeStableMerkleRootFromBranch(leaves[i], new_branch, pathmask.first, pathmask.second, nullptr));
            swap(hashZ, leaves[i]); // revert
        } else {
            // All of the remaining branches have at least one duplicated
            // hash.  The new-style branch is shorter than the old-style
            // branch because it does not include that hash.
            BOOST_CHECK_EQUAL(old_branch.size(), 5);
            switch (i) {
                case 16: // 0b10000
                case 17: // 0b10001
                case 18: // 0b10010
                case 19: // 0b10011
                    BOOST_CHECK_EQUAL(new_branch.size(), 4);
                    break;
                case 20: // 0b10100
                case 21: // 0b10101
                    BOOST_CHECK_EQUAL(new_branch.size(), 3);
                    break;
            }

            // And if we swap leaf values, only the new-style branch generates
            // correct root hashes.
            swap(leaves[i], hashZ);
            root = ComputeMerkleRoot(leaves, nullptr);
            BOOST_CHECK(root != ComputeMerkleRootFromBranch(leaves[i], old_branch, i));
            auto pathmask = ComputeMerklePathAndMask(new_branch.size(), i);
            BOOST_CHECK(root == ComputeStableMerkleRootFromBranch(leaves[i], new_branch, pathmask.first, pathmask.second, nullptr));
            swap(hashZ, leaves[i]); // revert
        }
    }
}

BOOST_AUTO_TEST_CASE(merkle_link)
{
    BOOST_CHECK(sizeof(MerkleLink) == 1);
}

BOOST_AUTO_TEST_CASE(merkle_node)
{
    BOOST_CHECK(sizeof(MerkleNode) == 1);

    BOOST_CHECK(MerkleNode().GetCode() == 0);

    const MerkleNode by_code[8] = {MerkleNode(0), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(4), MerkleNode(5), MerkleNode(6), MerkleNode(7)};
    const MerkleNode by_link[8] = {
        MerkleNode(MerkleLink::VERIFY, MerkleLink::SKIP),
        MerkleNode(MerkleLink::VERIFY, MerkleLink::VERIFY),
        MerkleNode(MerkleLink::VERIFY, MerkleLink::DESCEND),
        MerkleNode(MerkleLink::DESCEND, MerkleLink::SKIP),
        MerkleNode(MerkleLink::DESCEND, MerkleLink::VERIFY),
        MerkleNode(MerkleLink::DESCEND, MerkleLink::DESCEND),
        MerkleNode(MerkleLink::SKIP, MerkleLink::VERIFY),
        MerkleNode(MerkleLink::SKIP, MerkleLink::DESCEND),
    };

    for (int i = 0; i <= 7; ++i) {
        BOOST_CHECK(i == by_code[i].GetCode());
        BOOST_CHECK(i == by_link[i].GetCode());
    }

    for (int i = 0; i <= 7; ++i) {
        for (int j = 0; j <= 7; ++j) {
            BOOST_CHECK((i==j) == (by_code[i] == by_link[j]));
            BOOST_CHECK((i!=j) == (by_code[i] != by_link[j]));
            BOOST_CHECK((i<j) == (by_code[i] < by_link[j]));
            BOOST_CHECK((i<=j) == (by_code[i] <= by_link[j]));
            BOOST_CHECK((i>=j) == (by_code[i] >= by_link[j]));
            BOOST_CHECK((i>j) == (by_code[i] > by_link[j]));
        }
    }

    MerkleNode a(0);
    a.SetCode(1);
    BOOST_CHECK(a.GetCode() == 1);
    BOOST_CHECK(a == MerkleNode(1));

    a = MerkleNode(3);
    BOOST_CHECK(a != MerkleNode(1));
    BOOST_CHECK(a.GetCode() == 3);

    for (int i = 0; i <= 7; ++i) {
        MerkleNode n = by_code[i];
        MerkleLink l = n.GetLeft();
        MerkleLink r = n.GetRight();
        BOOST_CHECK(MerkleNode(l,r) == by_link[i]);
        for (int j = 0; j <= 2; ++j) {
          MerkleNode n2(n);
          BOOST_CHECK(n2 == n);
          n2.SetLeft(MerkleLink(j));
          BOOST_CHECK(n2 == MerkleNode(MerkleLink(j),r));
        }
        for (int j = 0; j <= 2; ++j) {
          MerkleNode n3(n);
          BOOST_CHECK(n3 == n);
          n3.SetRight(MerkleLink(j));
          BOOST_CHECK(n3 == MerkleNode(l,MerkleLink(j)));
        }
    }
}

/*
 * To properly test some features requires access to protected members
 * of these classes. In the case of MerkleNode, just some static class
 * members so we write a method to return those.
 */
struct PublicMerkleNode: public MerkleNode
{
    typedef const std::array<MerkleLink, 8> m_link_from_code_type;
    static m_link_from_code_type& m_left_from_code()
      { return MerkleNode::m_left_from_code; }
    static m_link_from_code_type& m_right_from_code()
      { return MerkleNode::m_right_from_code; }
};

/*
 * In the case of MerkleNodeReference we need access to class instance
 * members, so we have a somewhat more involve wrapper that is used
 * for actual MerkleNodeReference instances (and forwards whatever
 * functionality needs to be defined to the base class).
 */
struct PublicMerkleNodeReference: public MerkleNodeReference
{
    PublicMerkleNodeReference(base_type* base, offset_type offset) : MerkleNodeReference(base, offset) { }

    PublicMerkleNodeReference() = delete;

    PublicMerkleNodeReference(const PublicMerkleNodeReference& other) : MerkleNodeReference(other) { }
    PublicMerkleNodeReference(const MerkleNodeReference& other) : MerkleNodeReference(other) { }
    PublicMerkleNodeReference(PublicMerkleNodeReference&& other) : MerkleNodeReference(other) { }
    PublicMerkleNodeReference(MerkleNodeReference&& other) : MerkleNodeReference(other) { }
    inline PublicMerkleNodeReference& operator=(const PublicMerkleNodeReference& other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }
    inline PublicMerkleNodeReference& operator=(const MerkleNodeReference& other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }
    inline PublicMerkleNodeReference& operator=(PublicMerkleNodeReference&& other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }
    inline PublicMerkleNodeReference& operator=(MerkleNodeReference&& other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }

    inline PublicMerkleNodeReference& operator=(MerkleNode other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }
    inline operator MerkleNode() const
      { return MerkleNodeReference::operator MerkleNode(); }

    typedef MerkleNodeReference::base_type* m_base_type;
    m_base_type& m_base()
      { return MerkleNodeReference::m_base; }
    const m_base_type& m_base() const
      { return MerkleNodeReference::m_base; }

    typedef MerkleNodeReference::offset_type m_offset_type;
    m_offset_type& m_offset()
      { return MerkleNodeReference::m_offset; }
    const m_offset_type& m_offset() const
      { return MerkleNodeReference::m_offset; }
};

BOOST_AUTO_TEST_CASE(merkle_node_reference)
{
    MerkleNodeReference::base_type v[3] = {0};
    PublicMerkleNodeReference _r[8] = {
        {v, 0}, {v, 1}, {v, 2}, {v, 3},
        {v, 4}, {v, 5}, {v, 6}, {v, 7},
    };
    MerkleNodeReference* r = &_r[0];
    const MerkleNode n[8] = {
        MerkleNode(0), MerkleNode(1), MerkleNode(2), MerkleNode(3),
        MerkleNode(4), MerkleNode(5), MerkleNode(6), MerkleNode(7),
    };

    PublicMerkleNodeReference a(v, 0);
    BOOST_CHECK(a.m_base() == &v[0]);
    BOOST_CHECK(a.m_offset() == 0);

    for (int i = 0; i <= 7; ++i) {
        BOOST_CHECK(r[i].GetCode() == 0);
        PublicMerkleNodeReference(v, i).SetCode(i);
        BOOST_CHECK_MESSAGE(r[i].GetCode() == i, strprintf("%d", i).c_str());
    }

    BOOST_CHECK(v[0] == static_cast<MerkleNodeReference::base_type>(0x05));
    BOOST_CHECK(v[1] == static_cast<MerkleNodeReference::base_type>(0x39));
    BOOST_CHECK(v[2] == static_cast<MerkleNodeReference::base_type>(0x77));

    for (int i = 0; i <= 7; ++i) {
        BOOST_CHECK(n[i].GetCode() == i);
        BOOST_CHECK(r[i].GetCode() == i);
        BOOST_CHECK(r[i].GetLeft() == PublicMerkleNode::m_left_from_code()[i]);
        BOOST_CHECK(r[i].GetRight() == PublicMerkleNode::m_right_from_code()[i]);
    }

    PublicMerkleNodeReference ref(v, 0);
    PublicMerkleNodeReference ref2(v, 7);

    for (MerkleNode::code_type i = 0; i <= 7; ++i) {
        for (MerkleNode::code_type j = 0; j <= 7; ++j) {
          ref.SetCode(i);
          ref2.SetCode(j);
          MerkleNode node(j);
          BOOST_CHECK((i==j) == (ref == node));
          BOOST_CHECK((j==i) == (node == ref));
          BOOST_CHECK((i==j) == (ref == ref2));
          BOOST_CHECK((i!=j) == (ref != node));
          BOOST_CHECK((j!=i) == (node != ref));
          BOOST_CHECK((i!=j) == (ref != ref2));
          BOOST_CHECK((i<j) == (ref < node));
          BOOST_CHECK((j<i) == (node < ref));
          BOOST_CHECK((i<j) == (ref < ref2));
          BOOST_CHECK((i<=j) == (ref <= node));
          BOOST_CHECK((j<=i) == (node <= ref));
          BOOST_CHECK((i<=j) == (ref <= ref2));
          BOOST_CHECK((i>=j) == (ref >= node));
          BOOST_CHECK((j>=i) == (node >= ref));
          BOOST_CHECK((i>=j) == (ref >= ref2));
          BOOST_CHECK((i>j) == (ref > node));
          BOOST_CHECK((j>i) == (node > ref));
          BOOST_CHECK((i>j) == (ref > ref2));
          MerkleLink new_left = node.GetLeft();
          MerkleLink new_right = node.GetRight();
          if ((new_left == MerkleLink::SKIP) && (ref.GetRight() == MerkleLink::SKIP)) {
              /* Prevent errors due to temporary {SKIP,SKIP} */
              ref.SetRight(MerkleLink::VERIFY);
          }
          ref.SetLeft(new_left);
          BOOST_CHECK(ref.GetLeft() == node.GetLeft());
          if ((ref.GetLeft() == MerkleLink::SKIP) && (new_right == MerkleLink::SKIP)) {
              /* Prevent errors due to temporary {SKIP,SKIP} */
              ref.SetLeft(MerkleLink::VERIFY);
          }
          ref.SetRight(new_right);
          BOOST_CHECK(ref.GetRight() == node.GetRight());
          BOOST_CHECK(ref == node);
          BOOST_CHECK(node == ref);
          ref.SetCode(i);
          BOOST_CHECK((i==j) == (ref == ref2));
          ref2 = ref;
          BOOST_CHECK(ref == ref2);
          ref2 = node;
          BOOST_CHECK(ref2 == node);
          BOOST_CHECK((i==j) == (ref == ref2));
          static_cast<MerkleNode>(ref).SetCode(j);
          BOOST_CHECK((i==j) == (ref == ref2));
        }
    }
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_constructor)
{
    /* explicit vector(const Allocator& alloc = Allocator()) */
    std::vector<MerkleNode> def;
    BOOST_CHECK(def.empty());
    BOOST_CHECK(!def.dirty());

    std::allocator<MerkleNode> alloc;
    std::vector<MerkleNode> with_alloc(alloc);
    BOOST_CHECK(with_alloc.get_allocator() == std::allocator<MerkleNode>());
    BOOST_CHECK(with_alloc.get_allocator() == def.get_allocator());

    /* explicit vector(size_type count) */
    std::vector<MerkleNode> three(3);
    BOOST_CHECK(three.size() == 3);
    BOOST_CHECK(three[0] == MerkleNode());
    BOOST_CHECK(three[1] == MerkleNode());
    BOOST_CHECK(three[2] == MerkleNode());

    std::vector<MerkleNode> nine(9);
    BOOST_CHECK(nine.size() == 9);
    BOOST_CHECK(nine.front() == MerkleNode());
    BOOST_CHECK(nine.back() == MerkleNode());

    /* vector(size_type count, value_type value, const Allocator& alloc = Allocator()) */
    std::vector<MerkleNode> three_ones(3, MerkleNode(1));
    BOOST_CHECK(three_ones.size() == 3);
    BOOST_CHECK(three_ones[0] == MerkleNode(1));
    BOOST_CHECK(three_ones[1] == MerkleNode(1));
    BOOST_CHECK(three_ones[2] == MerkleNode(1));
    BOOST_CHECK(three.size() == three_ones.size());
    BOOST_CHECK(three != three_ones);

    std::vector<MerkleNode> nine_sevens(9, MerkleNode(7), alloc);
    BOOST_CHECK(nine_sevens.size() == 9);
    BOOST_CHECK(nine_sevens.front() == MerkleNode(7));
    BOOST_CHECK(nine_sevens.back() == MerkleNode(7));
    BOOST_CHECK(nine.size() == nine_sevens.size());
    BOOST_CHECK(nine != nine_sevens);

    /* void assign(size_type count, value_type value) */
    {
        std::vector<MerkleNode> t(nine_sevens);
        t.assign(3, MerkleNode(1));
        BOOST_CHECK(t == three_ones);
        std::vector<MerkleNode> t2(three_ones);
        t2.assign(9, MerkleNode(7));
        BOOST_CHECK(t2 == nine_sevens);
    }

    /* template<class InputIt> vector(InputIt first, InputIt last, const Allocator& alloc = Allocator()) */
    std::vector<MerkleNode> one_two_three;
    one_two_three.push_back(MerkleNode(1)); BOOST_CHECK(one_two_three[0].GetCode() == 1);
    one_two_three.push_back(MerkleNode(2)); BOOST_CHECK(one_two_three[1].GetCode() == 2);
    one_two_three.push_back(MerkleNode(3)); BOOST_CHECK(one_two_three[2].GetCode() == 3);

    std::list<MerkleNode> l{MerkleNode(1), MerkleNode(2), MerkleNode(3)};
    std::vector<MerkleNode> from_list(l.begin(), l.end());
    BOOST_CHECK(from_list == one_two_three);

    std::deque<MerkleNode> q{MerkleNode(3), MerkleNode(2), MerkleNode(1)};
    std::vector<MerkleNode> from_reversed_deque(q.rbegin(), q.rend(), alloc);
    BOOST_CHECK(from_reversed_deque == one_two_three);
    BOOST_CHECK(from_reversed_deque == from_list);

    /* template<class InputIt> void assign(InputIt first, InputIt last) */
    {
        std::vector<MerkleNode> t(nine_sevens);
        t.assign(from_list.begin(), from_list.end());
        BOOST_CHECK(t == one_two_three);
        std::vector<MerkleNode> t2;
        t2.assign(q.rbegin(), q.rend());
        BOOST_CHECK(t2 == one_two_three);
    }

    /* vector(std::initializer_list<value_type> ilist, const Allocator& alloc = Allocator()) */
    std::vector<MerkleNode> from_ilist{MerkleNode(1), MerkleNode(2), MerkleNode(3)};
    BOOST_CHECK(from_ilist == one_two_three);

    /* vector& operator=(std::initializer_list<value_type> ilist) */
    {
        std::vector<MerkleNode> t(nine_sevens);
        t = {MerkleNode(1), MerkleNode(2), MerkleNode(3)};
        BOOST_CHECK(t == one_two_three);
    }

    /* void assign(std::initializer_list<value_type> ilist) */
    {
        std::vector<MerkleNode> t(nine_sevens);
        t.assign({MerkleNode(1), MerkleNode(2), MerkleNode(3)});
        BOOST_CHECK(t == one_two_three);
    }

    /* vector(const vector& other) */
    {
        std::vector<MerkleNode> v123(one_two_three);
        BOOST_CHECK(v123.size() == 3);
        BOOST_CHECK(v123[0] == MerkleNode(1));
        BOOST_CHECK(v123[1] == MerkleNode(2));
        BOOST_CHECK(v123[2] == MerkleNode(3));
        BOOST_CHECK(v123 == one_two_three);
    }

    /* vector(const vector& other, const Allocator& alloc) */
    {
        std::vector<MerkleNode> v123(one_two_three, alloc);
        BOOST_CHECK(v123.size() == 3);
        BOOST_CHECK(v123[0] == MerkleNode(1));
        BOOST_CHECK(v123[1] == MerkleNode(2));
        BOOST_CHECK(v123[2] == MerkleNode(3));
        BOOST_CHECK(v123 == one_two_three);
    }

    /* vector(vector&& other) */
    {
        std::vector<MerkleNode> v123a(one_two_three);
        BOOST_CHECK(v123a == one_two_three);
        std::vector<MerkleNode> v123b(std::move(v123a));
        BOOST_CHECK(v123b == one_two_three);
    }

    /* vector(vector&& other, const Allocator& alloc) */
    {
        std::vector<MerkleNode> v123a(one_two_three);
        BOOST_CHECK(v123a == one_two_three);
        std::vector<MerkleNode> v123b(std::move(v123a), alloc);
        BOOST_CHECK(v123b == one_two_three);
    }

    /* vector& operator=(const vector& other) */
    {
        std::vector<MerkleNode> v123;
        v123 = one_two_three;
        BOOST_CHECK(v123.size() == 3);
        BOOST_CHECK(v123[0] == MerkleNode(1));
        BOOST_CHECK(v123[1] == MerkleNode(2));
        BOOST_CHECK(v123[2] == MerkleNode(3));
        BOOST_CHECK(v123 == one_two_three);
    }

    /* vector& operator=(vector&& other) */
    {
        std::vector<MerkleNode> v123;
        v123 = std::move(one_two_three);
        BOOST_CHECK(v123.size() == 3);
        BOOST_CHECK(v123[0] == MerkleNode(1));
        BOOST_CHECK(v123[1] == MerkleNode(2));
        BOOST_CHECK(v123[2] == MerkleNode(3));
    }
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_relational)
{
    std::vector<MerkleNode> a{MerkleNode(0)};
    std::vector<MerkleNode> b{MerkleNode(0), MerkleNode(1)};

    BOOST_CHECK(!(a==b));
    BOOST_CHECK(a!=b);
    BOOST_CHECK(a<b);
    BOOST_CHECK(a<=b);
    BOOST_CHECK(!(a>=b));
    BOOST_CHECK(!(a>b));

    a.push_back(MerkleNode(1));

    BOOST_CHECK(a==b);
    BOOST_CHECK(!(a!=b));
    BOOST_CHECK(!(a<b));
    BOOST_CHECK(a<=b);
    BOOST_CHECK(a>=b);
    BOOST_CHECK(!(a>b));

    a.push_back(MerkleNode(2));

    BOOST_CHECK(!(a==b));
    BOOST_CHECK(a!=b);
    BOOST_CHECK(!(a<b));
    BOOST_CHECK(!(a<=b));
    BOOST_CHECK(a>=b);
    BOOST_CHECK(a>b);
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_access)
{
    std::vector<MerkleNode> v{MerkleNode(1), MerkleNode(2), MerkleNode(3)};
    const std::vector<MerkleNode>& c = v;

    BOOST_CHECK(v == c);

    BOOST_CHECK(v.at(0) == MerkleNode(1));
    BOOST_CHECK(c.at(0) == MerkleNode(1));
    BOOST_CHECK(v.at(1) == MerkleNode(2));
    BOOST_CHECK(v.at(1) == MerkleNode(2));
    BOOST_CHECK(v.at(2) == MerkleNode(3));
    BOOST_CHECK(v.at(2) == MerkleNode(3));

    BOOST_CHECK_THROW(v.at(3), std::out_of_range);
    BOOST_CHECK_THROW(c.at(3), std::out_of_range);

    BOOST_CHECK(v[0] == MerkleNode(1));
    BOOST_CHECK(c[0] == MerkleNode(1));
    BOOST_CHECK(v[1] == MerkleNode(2));
    BOOST_CHECK(c[1] == MerkleNode(2));
    BOOST_CHECK(v[2] == MerkleNode(3));
    BOOST_CHECK(c[2] == MerkleNode(3));

    /* Known to work due to a weirdness of the packed format, used as
     * a check that the access is not bounds checked. */
    BOOST_CHECK(!v.dirty());
    BOOST_CHECK(v[3] == MerkleNode(0));
    BOOST_CHECK(!c.dirty());
    BOOST_CHECK(c[3] == MerkleNode(0));

    BOOST_CHECK(v.front() == MerkleNode(1));
    BOOST_CHECK(c.front() == MerkleNode(1));
    BOOST_CHECK(v.back() == MerkleNode(3));
    BOOST_CHECK(c.back() == MerkleNode(3));

    BOOST_CHECK(v.data()[0] == 0x29);
    BOOST_CHECK(c.data()[0] == 0x29);
    BOOST_CHECK(v.data()[1] == 0x80);
    BOOST_CHECK(c.data()[1] == 0x80);
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_iterator)
{
    std::vector<MerkleNode> v{MerkleNode(1), MerkleNode(2), MerkleNode(3)};
    const std::vector<MerkleNode>& cv = v;

    BOOST_CHECK(v.begin()[0] == MerkleNode(1));
    BOOST_CHECK(v.begin()[1] == MerkleNode(2));
    BOOST_CHECK(v.begin()[2] == MerkleNode(3));
    BOOST_CHECK(*(2 + v.begin()) == MerkleNode(3));
    auto i = v.begin();
    BOOST_CHECK(*i++ == MerkleNode(1));
    BOOST_CHECK(*i++ == MerkleNode(2));
    BOOST_CHECK(*i++ == MerkleNode(3));
    BOOST_CHECK(i-- == v.end());
    BOOST_CHECK(*i-- == MerkleNode(3));
    BOOST_CHECK(*i-- == MerkleNode(2));
    BOOST_CHECK(*i == MerkleNode(1));
    i += 2;
    BOOST_CHECK(*i == MerkleNode(3));
    BOOST_CHECK((i - v.begin()) == 2);
    i -= 2;
    BOOST_CHECK(i == v.begin());
    BOOST_CHECK(std::distance(v.begin(), v.end()) == 3);
    BOOST_CHECK((v.end() - v.begin()) == 3);

    BOOST_CHECK(cv.begin()[0] == MerkleNode(1));
    BOOST_CHECK(cv.begin()[1] == MerkleNode(2));
    BOOST_CHECK(cv.begin()[2] == MerkleNode(3));
    BOOST_CHECK(*(2 + cv.begin()) == MerkleNode(3));
    auto c = cv.begin();
    BOOST_CHECK(*c++ == MerkleNode(1));
    BOOST_CHECK(*c++ == MerkleNode(2));
    BOOST_CHECK(*c++ == MerkleNode(3));
    BOOST_CHECK(c-- == cv.cend());
    BOOST_CHECK(*c-- == MerkleNode(3));
    BOOST_CHECK(*c-- == MerkleNode(2));
    BOOST_CHECK(*c == MerkleNode(1));
    c += 2;
    BOOST_CHECK(*c == MerkleNode(3));
    BOOST_CHECK((c - v.begin()) == 2);
    c -= 2;
    BOOST_CHECK(c == cv.begin());
    BOOST_CHECK(std::distance(cv.begin(), cv.end()) == 3);
    BOOST_CHECK((cv.end() - cv.begin()) == 3);

    BOOST_CHECK(v.cbegin()[0] == MerkleNode(1));
    BOOST_CHECK(v.cbegin()[1] == MerkleNode(2));
    BOOST_CHECK(v.cbegin()[2] == MerkleNode(3));
    BOOST_CHECK(*(2 + v.cbegin()) == MerkleNode(3));
    auto c2 = v.cbegin();
    BOOST_CHECK(*c2++ == MerkleNode(1));
    BOOST_CHECK(*c2++ == MerkleNode(2));
    BOOST_CHECK(*c2++ == MerkleNode(3));
    BOOST_CHECK(c2-- == v.cend());
    BOOST_CHECK(*c2-- == MerkleNode(3));
    BOOST_CHECK(*c2-- == MerkleNode(2));
    BOOST_CHECK(*c2 == MerkleNode(1));
    c2 += 2;
    BOOST_CHECK(*c2 == MerkleNode(3));
    BOOST_CHECK((c2 - v.cbegin()) == 2);
    c2 -= 2;
    BOOST_CHECK(c2 == v.cbegin());
    BOOST_CHECK(std::distance(v.cbegin(), v.cend()) == 3);
    BOOST_CHECK((v.cend() - v.cbegin()) == 3);

    BOOST_CHECK(v.rbegin()[0] == MerkleNode(3));
    BOOST_CHECK(v.rbegin()[1] == MerkleNode(2));
    BOOST_CHECK(v.rbegin()[2] == MerkleNode(1));
    BOOST_CHECK(*(2 + v.rbegin()) == MerkleNode(1));
    auto r = v.rbegin();
    BOOST_CHECK(*r++ == MerkleNode(3));
    BOOST_CHECK(*r++ == MerkleNode(2));
    BOOST_CHECK(*r++ == MerkleNode(1));
    BOOST_CHECK(r-- == v.rend());
    BOOST_CHECK(*r-- == MerkleNode(1));
    BOOST_CHECK(*r-- == MerkleNode(2));
    BOOST_CHECK(*r == MerkleNode(3));
    r += 2;
    BOOST_CHECK(*r == MerkleNode(1));
    BOOST_CHECK((r - v.rbegin()) == 2);
    r -= 2;
    BOOST_CHECK(r == v.rbegin());
    BOOST_CHECK(std::distance(v.rbegin(), v.rend()) == 3);
    BOOST_CHECK((v.rend() - v.rbegin()) == 3);

    BOOST_CHECK(cv.rbegin()[0] == MerkleNode(3));
    BOOST_CHECK(cv.rbegin()[1] == MerkleNode(2));
    BOOST_CHECK(cv.rbegin()[2] == MerkleNode(1));
    BOOST_CHECK(*(2 + cv.rbegin()) == MerkleNode(1));
    auto rc = cv.rbegin();
    BOOST_CHECK(*rc++ == MerkleNode(3));
    BOOST_CHECK(*rc++ == MerkleNode(2));
    BOOST_CHECK(*rc++ == MerkleNode(1));
    BOOST_CHECK(rc-- == cv.rend());
    BOOST_CHECK(*rc-- == MerkleNode(1));
    BOOST_CHECK(*rc-- == MerkleNode(2));
    BOOST_CHECK(*rc == MerkleNode(3));
    rc += 2;
    BOOST_CHECK(*rc == MerkleNode(1));
    BOOST_CHECK((rc - cv.rbegin()) == 2);
    rc -= 2;
    BOOST_CHECK(rc == cv.rbegin());
    BOOST_CHECK(std::distance(cv.rbegin(), cv.rend()) == 3);
    BOOST_CHECK((cv.rend() - cv.rbegin()) == 3);

    BOOST_CHECK(v.crbegin()[0] == MerkleNode(3));
    BOOST_CHECK(v.crbegin()[1] == MerkleNode(2));
    BOOST_CHECK(v.crbegin()[2] == MerkleNode(1));
    BOOST_CHECK(*(2 + v.crbegin()) == MerkleNode(1));
    auto rc2 = v.crbegin();
    BOOST_CHECK(*rc2++ == MerkleNode(3));
    BOOST_CHECK(*rc2++ == MerkleNode(2));
    BOOST_CHECK(*rc2++ == MerkleNode(1));
    BOOST_CHECK(rc2-- == v.crend());
    BOOST_CHECK(*rc2-- == MerkleNode(1));
    BOOST_CHECK(*rc2-- == MerkleNode(2));
    BOOST_CHECK(*rc2 == MerkleNode(3));
    rc2 += 2;
    BOOST_CHECK(*rc2 == MerkleNode(1));
    BOOST_CHECK((rc2 - v.crbegin()) == 2);
    rc2 -= 2;
    BOOST_CHECK(rc2 == v.crbegin());
    BOOST_CHECK(std::distance(v.crbegin(), v.crend()) == 3);
    BOOST_CHECK((v.crend() - v.crbegin()) == 3);
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_capacity)
{
    std::vector<MerkleNode> v;
    BOOST_CHECK(v.empty());
    BOOST_CHECK(v.size() == 0);
    BOOST_CHECK(v.max_size() >= std::vector<unsigned char>().max_size());
    BOOST_CHECK(v.capacity() >= v.size());

    v.push_back(MerkleNode(1));
    BOOST_CHECK(!v.empty());
    BOOST_CHECK(v.size() == 1);
    BOOST_CHECK(v.capacity() >= v.size());

    v.push_back(MerkleNode(2));
    BOOST_CHECK(!v.empty());
    BOOST_CHECK(v.size() == 2);
    BOOST_CHECK(v.capacity() >= v.size());

    v.push_back(MerkleNode(3));
    BOOST_CHECK(!v.empty());
    BOOST_CHECK(v.size() == 3);
    BOOST_CHECK(v.capacity() >= v.size());

    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3)}));

    v.resize(6);
    BOOST_CHECK(!v.empty());
    BOOST_CHECK(v.size() == 6);
    BOOST_CHECK(v.capacity() >= v.size());

    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                              MerkleNode(0), MerkleNode(0), MerkleNode(0)}));

    v.shrink_to_fit();
    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                              MerkleNode(0), MerkleNode(0), MerkleNode(0)}));

    v.resize(9, MerkleNode(7));
    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                              MerkleNode(0), MerkleNode(0), MerkleNode(0),
                                              MerkleNode(7), MerkleNode(7), MerkleNode(7)}));

    v.shrink_to_fit();
    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                              MerkleNode(0), MerkleNode(0), MerkleNode(0),
                                              MerkleNode(7), MerkleNode(7), MerkleNode(7)}));

    v.resize(3);
    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
}

/*
 * Takes an iterator of any category derived from InputIterator, and
 * exposes an interface restricted to only support the InputIterator
 * API, including the input_iterator_tag. Useful for mocking an
 * InputIterator for testing non-random-access or non-bidirectional
 * code paths.
 */
template<class Iter>
struct mock_input_iterator
{
    typedef Iter inner_type;
    inner_type m_iter;

    typedef mock_input_iterator iterator;
    typedef std::input_iterator_tag iterator_category;
    typedef typename Iter::value_type value_type;
    typedef typename Iter::difference_type difference_type;
    typedef typename Iter::pointer pointer;
    typedef typename Iter::reference reference;

    mock_input_iterator() = delete;

    explicit mock_input_iterator(inner_type& iter) : m_iter(iter) { }

    iterator& operator=(inner_type& iter)
    {
        m_iter = iter;
        return *this;
    }

    mock_input_iterator(const iterator&) = default;
    mock_input_iterator(iterator&&) = default;
    iterator& operator=(const iterator&) = default;
    iterator& operator=(iterator&&) = default;

    /* Distance */
    difference_type operator-(const iterator& other) const
      { return (m_iter - other.m_iter); }

    /* Equality */
    inline bool operator==(const iterator& other) const
      { return (m_iter == other.m_iter); }
    inline bool operator!=(const iterator& other) const
      { return (m_iter != other.m_iter); }

    /* Input iterators are not relational comparable */

    /* Dereference */
    inline reference operator*() const
      { return (*m_iter); }
    inline pointer operator->() const
      { return m_iter.operator->(); }

    /* Advancement */
    inline iterator& operator++()
    {
        m_iter.operator++();
        return *this;
    }

    inline iterator& operator++(int _)
      { return iterator(m_iter.operator++(_)); }
};

template<class Iter>
inline mock_input_iterator<Iter> wrap_mock_input_iterator(Iter iter)
  { return mock_input_iterator<Iter>(iter); }

BOOST_AUTO_TEST_CASE(merkle_node_vector_insert)
{
    /* void push_back(value_type value) */
    /* template<class... Args> void emplace_back(Args&&... args) */
    std::vector<MerkleNode> one_two_three;
    one_two_three.push_back(MerkleNode(1));
    one_two_three.emplace_back(static_cast<MerkleNode::code_type>(2));
    one_two_three.emplace_back(MerkleLink::DESCEND, MerkleLink::SKIP);
    BOOST_CHECK(one_two_three.size() == 3);
    BOOST_CHECK(one_two_three[0] == MerkleNode(1));
    BOOST_CHECK(one_two_three[1] == MerkleNode(2));
    BOOST_CHECK(one_two_three[2] == MerkleNode(3));

    /* void clear() */
    {
        std::vector<MerkleNode> v(one_two_three);
        BOOST_CHECK(v.size() == 3);
        BOOST_CHECK(v == one_two_three);
        v.clear();
        BOOST_CHECK(v.empty());
        BOOST_CHECK(one_two_three.size() == 3);
    }

    /* iterator insert(const_iterator pos, value_type value) */
    {
        std::vector<MerkleNode> v(one_two_three);
        auto res = v.insert(v.begin(), MerkleNode(0));
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v != one_two_three);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
        res = v.insert(v.begin()+2, MerkleNode(4));
        BOOST_CHECK(res == (v.begin()+2));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(3)}));
        res = v.insert(v.begin()+4, MerkleNode(5));
        BOOST_CHECK(res == (v.begin()+4));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(3)}));
        res = v.insert(v.begin()+6, MerkleNode(6));
        BOOST_CHECK(res == (v.begin()+6));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(3), MerkleNode(6)}));
        res = v.insert(v.end(), MerkleNode(7));
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+7));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(3), MerkleNode(6), MerkleNode(7)}));
    }

    /* iterator insert(const_iterator pos, size_type count, value_type value) */
    {
        std::vector<MerkleNode> v(one_two_three);
        auto res = v.insert(v.begin(), 0, MerkleNode(0));
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == one_two_three);
        res = v.insert(v.begin()+1, 1, MerkleNode(4));
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(3)}));
        res = v.insert(v.begin()+3, 2, MerkleNode(5));
        BOOST_CHECK(res == (v.begin()+3));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(5), MerkleNode(3)}));
        res = v.insert(v.begin()+6, 3, MerkleNode(6));
        BOOST_CHECK(res == (v.begin()+6));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(5), MerkleNode(3), MerkleNode(6), MerkleNode(6), MerkleNode(6)}));
        res = v.insert(v.end(), 2, MerkleNode(7));
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+9));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(5), MerkleNode(3), MerkleNode(6), MerkleNode(6), MerkleNode(6), MerkleNode(7), MerkleNode(7)}));
    }

    /* template<class InputIt> iterator insert(const_iterator pos, InputIt first, InputIt last) */
    {
        std::vector<MerkleNode> ones({MerkleNode(1), MerkleNode(1)});
        std::vector<MerkleNode> twos({MerkleNode(2), MerkleNode(2)});
        std::vector<MerkleNode> v;
        BOOST_CHECK(v.empty());
        auto res = v.insert(v.begin(), wrap_mock_input_iterator(ones.begin()), wrap_mock_input_iterator(ones.end()));
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1)}));
        res = v.insert(v.begin()+1, one_two_three.begin(), one_two_three.end());
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1)}));
        res = v.insert(v.end(), twos.begin(), twos.begin() + 1);
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+5));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2)}));
        std::vector<MerkleNode> v2(v);
        res = v2.insert(v2.end(), v.begin(), v.end());
        BOOST_CHECK(res != v2.end());
        BOOST_CHECK(res == (v2.begin()+6));
        BOOST_CHECK(v2 == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2)}));
        std::vector<MerkleNode> v3(v2);
        res = v3.insert(v3.begin()+1, v2.begin(), v2.end());
        BOOST_CHECK(res == (v3.begin()+1));
        BOOST_CHECK(v3 == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2)}));
        res = v3.insert(v3.begin(), one_two_three.begin(), one_two_three.end());
        BOOST_CHECK(res == v3.begin());
        BOOST_CHECK(v3 == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2)}));
    }

    /* iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) */
    {
        std::vector<MerkleNode> v;
        auto res = v.insert(v.begin(), {MerkleNode(1), MerkleNode(1)});
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1)}));
        res = v.insert(v.begin()+1, {MerkleNode(2), MerkleNode(2)});
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(2), MerkleNode(1)}));
        res = v.insert(v.end(), {MerkleNode(3)});
        BOOST_CHECK(res == (v.begin()+4));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(2), MerkleNode(1), MerkleNode(3)}));
    }

    /* template<class... Args> iterator emplace(const_iterator pos, Args&&... args) */
    {
        std::vector<MerkleNode> v;
        auto res = v.emplace(v.begin());
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0)}));
        res = v.emplace(v.end(), MerkleNode(2));
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(2)}));
        res = v.emplace(res, static_cast<MerkleNode::code_type>(1));
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(2)}));
        res = v.emplace(v.end(), MerkleLink::DESCEND, MerkleLink::SKIP);
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+3));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
    }

    /* iterator erase(const_iterator pos) */
    {
        std::vector<MerkleNode> v(one_two_three);
        auto res = v.erase(v.begin()+1);
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(3)}));
        res = v.erase(v.begin());
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(3)}));
    }

    /* iterator erase(const_iterator first, const_iterator last) */
    {
        std::vector<MerkleNode> v(one_two_three);
        auto res = v.erase(v.begin()+1, v.end());
        BOOST_CHECK(res == v.end());
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1)}));
    }

    /* void pop_back() */
    {
        std::vector<MerkleNode> v;
        v.insert(v.end(), one_two_three.begin(), one_two_three.end());
        v.insert(v.end(), one_two_three.begin(), one_two_three.end());
        v.insert(v.end(), one_two_three.begin(), one_two_three.end());
        BOOST_CHECK(v.size() == 9);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 8);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 7);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 6);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 5);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 4);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 3);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 2);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 1);
        BOOST_CHECK(v == std::vector<MerkleNode>(1, MerkleNode(1)));
        v.pop_back();
        BOOST_CHECK(v.empty());
        BOOST_CHECK(v == std::vector<MerkleNode>());
    }

    /* void swap(vector& other) */
    {
        std::vector<MerkleNode> tmp;
        BOOST_CHECK(tmp.empty());
        tmp.swap(one_two_three);
        BOOST_CHECK(tmp.size() == 3);
    }
    BOOST_CHECK(one_two_three.empty());
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_dirty)
{
    static std::size_t k_vch_size[9] =
      {0, 1, 1, 2, 2, 2, 3, 3, 3};

    for (std::size_t i = 1; i <= 8; ++i) {
        std::vector<MerkleNode> v;
        for (std::size_t j = 0; j < i; ++j)
            v.emplace_back(static_cast<MerkleNode::code_type>(j));
        for (std::size_t j = 0; j < (8*k_vch_size[i]); ++j) {
            BOOST_CHECK(!v.dirty());
            v.data()[j/8] ^= (1 << (7-(j%8)));
            BOOST_CHECK((j < (3*i)) == !(v.dirty()));
            v.data()[j/8] ^= (1 << (7-(j%8)));
        }
    }
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_serialize)
{
    std::vector<MerkleNode> v;
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "00");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(0);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "0100");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(1);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "0204");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(2);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "030500");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(3);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "040530");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(4);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "050538");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(5);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "06053940");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(6);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "07053970");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(7);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "08053977");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(5);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << v;
        BOOST_CHECK_EQUAL(HexStr(ds), "09053977a0");
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    {
        auto data = ParseHex("02600239361160903c6695c6804b7157c7bd10013e9ba89b1f954243bc8e3990b08db96632753d6ca30fea890f37fc150eaed8d068acf596acb2251b8fafd72db977d3");
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << Span<unsigned char>(data.data(), data.data() + data.size());
        MerkleProof proof;
        BOOST_CHECK_MESSAGE(ds[0] == std::byte{0x02}, HexStr(ds).c_str());
        BOOST_CHECK_MESSAGE(ds.size() == 67, HexStr(ds).c_str());
        ds >> proof;
        BOOST_CHECK(ds.empty());
        BOOST_CHECK(proof.m_path.size() == 2);
        BOOST_CHECK(proof.m_path[0] == MerkleNode(MerkleLink::DESCEND, MerkleLink::SKIP));
        BOOST_CHECK(proof.m_path[1] == MerkleNode(MerkleLink::VERIFY, MerkleLink::SKIP));
        BOOST_CHECK(proof.m_skip.size() == 2);
        BOOST_CHECK(proof.m_skip[0] == uint256S("b98db090398ebc4342951f9ba89b3e0110bdc757714b80c695663c9060113639"));
        BOOST_CHECK(proof.m_skip[1] == uint256S("d377b92dd7af8f1b25b2ac96f5ac68d0d8ae0e15fc370f89ea0fa36c3d753266"));
    }
}

BOOST_AUTO_TEST_CASE(merkle_tree_constructor)
{
    uint256 hashZero;
    CHash256().Finalize(hashZero);
    BOOST_CHECK(hashZero == uint256S("56944c5d3f98413ef45cf54545538103cc9f298e0575820ad3591376e2e0f65d"));

    uint256 hashA;
    CHash256().Write({(const unsigned char*)"A", 1}).Finalize(hashA);
    BOOST_CHECK(hashA == uint256S("425ea523fee4a4451246a49a08174424ee3fdc03d40926ad46ffe0e671efd61c"));

    uint256 hashB;
    CHash256().Write({(const unsigned char*)"B", 1}).Finalize(hashB);
    BOOST_CHECK(hashB == uint256S("01517aea572935ff9eb1455bc1147f98fb60957f4f9f868f06824ede3bb0550b"));

    uint256 hashC;
    CHash256().Write({(const unsigned char*)"C", 1}).Finalize(hashC);
    BOOST_CHECK(hashC == uint256S("ea3f6455fc84430d6f2db40d708a046caab99ad8207d14e43b2f1ffd68894fca"));

    uint256 hashD;
    CHash256().Write({(const unsigned char*)"D", 1}).Finalize(hashD);
    BOOST_CHECK(hashD == uint256S("2e52efc7b8cab2e0ca3f688ae090febff94be0eaa3ce666301985b287fc6e178"));

    uint256 hashE;
    CHash256().Write({(const unsigned char*)"E", 1}).Finalize(hashE);
    BOOST_CHECK(hashE == uint256S("a9c6b81b74f77d73def7397879bd23301159ce9554b2be00b09a2bab0c033c2d"));

    uint256 hashF;
    CHash256().Write({(const unsigned char*)"F", 1}).Finalize(hashF);
    BOOST_CHECK(hashF == uint256S("1c4a32d1d781dd8633c2c21af8b24c6219278f5ea89adf2ee053c276b55a1f42"));

    bool invalid = true;
    std::vector<MerkleBranch> branches;

    MerkleTree zero;
    BOOST_CHECK(zero.m_proof.m_path.empty());
    BOOST_CHECK(zero.m_proof.m_skip.empty());
    BOOST_CHECK(zero.m_verify.empty());
    invalid = true;
    branches.clear();
    BOOST_CHECK(zero.GetHash(&invalid, &branches) == hashZero);
    BOOST_CHECK(!invalid);
    BOOST_CHECK(branches.empty());

    MerkleTree verify(hashA);
    BOOST_CHECK(verify.m_proof.m_path.empty());
    BOOST_CHECK(verify.m_proof.m_skip.empty());
    BOOST_CHECK(verify.m_verify.size() == 1);
    BOOST_CHECK(verify.m_verify[0] == hashA);
    invalid = true;
    branches.clear();
    BOOST_CHECK(verify.GetHash(&invalid, &branches) == hashA);
    BOOST_CHECK(!invalid);
    BOOST_CHECK(branches.size() == 1);
    BOOST_CHECK(branches[0].m_branch.empty());
    BOOST_CHECK(branches[0].m_vpath.empty());
    invalid = true;
    BOOST_CHECK(ComputeFastMerkleRootFromBranch(verify.m_verify[0], branches[0].m_branch, branches[0].GetPath(), &invalid) == hashA);
    BOOST_CHECK(!invalid);

    MerkleTree skip(hashB, false);
    BOOST_CHECK(skip.m_proof.m_path.empty());
    BOOST_CHECK(skip.m_proof.m_skip.size() == 1);
    BOOST_CHECK(skip.m_proof.m_skip[0] == hashB);
    BOOST_CHECK(skip.m_verify.empty());
    invalid = true;
    branches.clear();
    BOOST_CHECK(skip.GetHash(&invalid, &branches) == hashB);
    BOOST_CHECK(!invalid);
    BOOST_CHECK(branches.empty());

    /* Constructing with an empty tree is idempotent. */
    BOOST_CHECK(MerkleTree(zero, zero) == zero);
    BOOST_CHECK(MerkleTree(zero, verify) == verify);
    BOOST_CHECK(MerkleTree(verify, zero) == verify);
    BOOST_CHECK(MerkleTree(zero, skip) == skip);
    BOOST_CHECK(MerkleTree(skip, zero) == skip);

    /* Two items: [A B].
     * We'll enumerate the possible combination of VERIFY and SKIP
     * hashes. */
    uint256 hashAB;
    MerkleHash_Sha256Midstate(hashAB, hashA, hashB);

    std::vector<MerkleBranch> branchesAB = {
        { {hashB}, {false} },
        { {hashA}, {true} },
    };

    {
        /* 0 */
        auto res = MerkleTree(
            MerkleTree(hashA),
            MerkleTree(hashB));
        BOOST_CHECK(res.m_proof.m_path.size() == 1);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.empty());
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches == branchesAB);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 1 */
        auto res = MerkleTree(
            MerkleTree(hashA, false),
            MerkleTree(hashB));
        BOOST_CHECK(res.m_proof.m_path.size() == 1);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashB);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 2 */
        auto res = MerkleTree(
            MerkleTree(hashA),
            MerkleTree(hashB, false));
        BOOST_CHECK(res.m_proof.m_path.size() == 1);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashA);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB[0]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 3 */
        auto res = MerkleTree(
            MerkleTree(hashA, false),
            MerkleTree(hashB, false));
        BOOST_CHECK(res.m_proof.m_path.empty());
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB);
        BOOST_CHECK(res.m_verify.empty());
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches.empty());
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    /* Three items: [[A B] C]. */
    uint256 hashAB_C;
    MerkleHash_Sha256Midstate(hashAB_C, hashAB, hashC);

    std::vector<MerkleBranch> branchesAB_C = {
        { {hashB, hashC}, {false, false} },
        { {hashA, hashC}, {true,  false} },
        { {hashAB},       {true}         },
    };

    {
        /* 0 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB)),
            MerkleTree(hashC));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.empty());
        BOOST_CHECK(res.m_verify.size() == 3);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches == branchesAB_C);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 1 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB)),
            MerkleTree(hashC));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashB);
        BOOST_CHECK(res.m_verify[1] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C[1]);
        BOOST_CHECK(branches[1] == branchesAB_C[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 2 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB, false)),
            MerkleTree(hashC));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C[0]);
        BOOST_CHECK(branches[1] == branchesAB_C[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 3 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB, false)),
            MerkleTree(hashC));
        BOOST_CHECK(res.m_proof.m_path.size() == 1);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 4 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB)),
            MerkleTree(hashC, false));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashC);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C[0]);
        BOOST_CHECK(branches[1] == branchesAB_C[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 5 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB)),
            MerkleTree(hashC, false));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashB);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 6 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB, false)),
            MerkleTree(hashC, false));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashA);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C[0]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 7 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB, false)),
            MerkleTree(hashC, false)
        );
        BOOST_CHECK(res.m_proof.m_path.empty());
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C);
        BOOST_CHECK(res.m_verify.empty());
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches.empty());
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    /* Three items: [D [E F]]. */
    uint256 hashEF;
    MerkleHash_Sha256Midstate(hashEF, hashE, hashF);
    uint256 hashD_EF;
    MerkleHash_Sha256Midstate(hashD_EF, hashD, hashEF);

    std::vector<MerkleBranch> branchesD_EF = {
        { {hashEF},       {false}       },
        { {hashF, hashD}, {false, true} },
        { {hashE, hashD}, {true,  true} },
    };

    {
        /* 0 */
        auto res = MerkleTree(
            MerkleTree(hashD),
            MerkleTree(
                MerkleTree(hashE),
                MerkleTree(hashF)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.empty());
        BOOST_CHECK(res.m_verify.size() == 3);
        BOOST_CHECK(res.m_verify[0] == hashD);
        BOOST_CHECK(res.m_verify[1] == hashE);
        BOOST_CHECK(res.m_verify[2] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashD_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches == branchesD_EF);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 1 */
        auto res = MerkleTree(
            MerkleTree(hashD, false),
            MerkleTree(
                MerkleTree(hashE),
                MerkleTree(hashF)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashE);
        BOOST_CHECK(res.m_verify[1] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashD_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesD_EF[1]);
        BOOST_CHECK(branches[1] == branchesD_EF[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 2 */
        auto res = MerkleTree(
            MerkleTree(hashD),
            MerkleTree(
                MerkleTree(hashE, false),
                MerkleTree(hashF)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashE);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashD);
        BOOST_CHECK(res.m_verify[1] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashD_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesD_EF[0]);
        BOOST_CHECK(branches[1] == branchesD_EF[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 3 */
        auto res = MerkleTree(
            MerkleTree(hashD, false),
            MerkleTree(
                MerkleTree(hashE, false),
                MerkleTree(hashF)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashE);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashD_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesD_EF[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 4 */
        auto res = MerkleTree(
            MerkleTree(hashD),
            MerkleTree(
                MerkleTree(hashE),
                MerkleTree(hashF, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashF);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashD);
        BOOST_CHECK(res.m_verify[1] == hashE);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashD_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesD_EF[0]);
        BOOST_CHECK(branches[1] == branchesD_EF[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 5 */
        auto res = MerkleTree(
            MerkleTree(hashD, false),
            MerkleTree(
                MerkleTree(hashE),
                MerkleTree(hashF, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashF);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashE);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashD_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesD_EF[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 6 */
        auto res = MerkleTree(
            MerkleTree(hashD),
            MerkleTree(
                MerkleTree(hashE, false),
                MerkleTree(hashF, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 1);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashEF);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashD_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesD_EF[0]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 7 */
        auto res = MerkleTree(
            MerkleTree(hashD, false),
            MerkleTree(
                MerkleTree(hashE, false),
                MerkleTree(hashF, false)));
        BOOST_CHECK(res.m_proof.m_path.empty());
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD_EF);
        BOOST_CHECK(res.m_verify.empty());
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashD_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches.empty());
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    /* Four items: [[A B] [C D]]. */
    uint256 hashCD;
    MerkleHash_Sha256Midstate(hashCD, hashC, hashD);
    uint256 hashAB_CD;
    MerkleHash_Sha256Midstate(hashAB_CD, hashAB, hashCD);

    std::vector<MerkleBranch> branchesAB_CD = {
        { {hashB, hashCD}, {false, false} },
        { {hashA, hashCD}, {true,  false} },
        { {hashD, hashAB}, {false, true}  },
        { {hashC, hashAB}, {true,  true}  },
    };

    {
        /* 0 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB)),
            MerkleTree(
                MerkleTree(hashC),
                MerkleTree(hashD)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.empty());
        BOOST_CHECK(res.m_verify.size() == 4);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        BOOST_CHECK(res.m_verify[3] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches == branchesAB_CD);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 1 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB)),
            MerkleTree(
                MerkleTree(hashC),
                MerkleTree(hashD)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_verify.size() == 3);
        BOOST_CHECK(res.m_verify[0] == hashB);
        BOOST_CHECK(res.m_verify[1] == hashC);
        BOOST_CHECK(res.m_verify[2] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 3);
        BOOST_CHECK(branches[0] == branchesAB_CD[1]);
        BOOST_CHECK(branches[1] == branchesAB_CD[2]);
        BOOST_CHECK(branches[2] == branchesAB_CD[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 2 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB, false)),
            MerkleTree(
                MerkleTree(hashC),
                MerkleTree(hashD)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_verify.size() == 3);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashC);
        BOOST_CHECK(res.m_verify[2] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 3);
        BOOST_CHECK(branches[0] == branchesAB_CD[0]);
        BOOST_CHECK(branches[1] == branchesAB_CD[2]);
        BOOST_CHECK(branches[2] == branchesAB_CD[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 3 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB, false)),
            MerkleTree(
                MerkleTree(hashC),
                MerkleTree(hashD)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashC);
        BOOST_CHECK(res.m_verify[1] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_CD[2]);
        BOOST_CHECK(branches[1] == branchesAB_CD[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 4 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB)),
            MerkleTree(
                MerkleTree(hashC, false),
                MerkleTree(hashD)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashC);
        BOOST_CHECK(res.m_verify.size() == 3);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 3);
        BOOST_CHECK(branches[0] == branchesAB_CD[0]);
        BOOST_CHECK(branches[1] == branchesAB_CD[1]);
        BOOST_CHECK(branches[2] == branchesAB_CD[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 5 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB)),
            MerkleTree(
                MerkleTree(hashC, false),
                MerkleTree(hashD)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashB);
        BOOST_CHECK(res.m_verify[1] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_CD[1]);
        BOOST_CHECK(branches[1] == branchesAB_CD[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 6 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB, false)),
            MerkleTree(
                MerkleTree(hashC, false),
                MerkleTree(hashD)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_CD[0]);
        BOOST_CHECK(branches[1] == branchesAB_CD[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 7 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB, false)),
            MerkleTree(
                MerkleTree(hashC, false),
                MerkleTree(hashD)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_CD[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 8 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB)),
            MerkleTree(
                MerkleTree(hashC),
                MerkleTree(hashD, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD);
        BOOST_CHECK(res.m_verify.size() == 3);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 3);
        BOOST_CHECK(branches[0] == branchesAB_CD[0]);
        BOOST_CHECK(branches[1] == branchesAB_CD[1]);
        BOOST_CHECK(branches[2] == branchesAB_CD[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 9 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB)),
            MerkleTree(
                MerkleTree(hashC),
                MerkleTree(hashD, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashB);
        BOOST_CHECK(res.m_verify[1] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_CD[1]);
        BOOST_CHECK(branches[1] == branchesAB_CD[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 10 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB, false)),
            MerkleTree(
                MerkleTree(hashC),
                MerkleTree(hashD, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_CD[0]);
        BOOST_CHECK(branches[1] == branchesAB_CD[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 11 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB, false)),
            MerkleTree(
                MerkleTree(hashC),
                MerkleTree(hashD, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_CD[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 12 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB)),
            MerkleTree(
                MerkleTree(hashC, false),
                MerkleTree(hashD, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashCD);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_CD[0]);
        BOOST_CHECK(branches[1] == branchesAB_CD[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 13 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB)),
            MerkleTree(
                MerkleTree(hashC, false),
                MerkleTree(hashD, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashCD);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashB);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_CD[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 14 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA),
                MerkleTree(hashB, false)),
            MerkleTree(
                MerkleTree(hashC, false),
                MerkleTree(hashD, false)));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashCD);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashA);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_CD[0]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 15 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(hashA, false),
                MerkleTree(hashB, false)),
            MerkleTree(
                MerkleTree(hashC, false),
                MerkleTree(hashD, false)));
        BOOST_CHECK(res.m_proof.m_path.empty());
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_CD);
        BOOST_CHECK(res.m_verify.empty());
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_CD);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches.empty());
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    /* Finally, a particular combination of six items:
     * [[[A B] C] [D [E F]]]. */
    uint256 hashAB_C__D_EF;
    MerkleHash_Sha256Midstate(hashAB_C__D_EF, hashAB_C, hashD_EF);

    std::vector<MerkleBranch> branchesAB_C__D_EF = {
        { {hashB,  hashC, hashD_EF}, {false, false, false} },
        { {hashA,  hashC, hashD_EF}, {true,  false, false} },
        { {hashAB, hashD_EF},        {true,  false}        },
        { {hashEF, hashAB_C},        {false, true}         },
        { {hashF,  hashD, hashAB_C}, {false, true,  true}  },
        { {hashE,  hashD, hashAB_C}, {true,  true,  true}  },
    };

    {
        /* 0 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.empty());
        BOOST_CHECK(res.m_verify.size() == 6);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        BOOST_CHECK(res.m_verify[3] == hashD);
        BOOST_CHECK(res.m_verify[4] == hashE);
        BOOST_CHECK(res.m_verify[5] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches == branchesAB_C__D_EF);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 1 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_verify.size() == 5);
        BOOST_CHECK(res.m_verify[0] == hashB);
        BOOST_CHECK(res.m_verify[1] == hashC);
        BOOST_CHECK(res.m_verify[2] == hashD);
        BOOST_CHECK(res.m_verify[3] == hashE);
        BOOST_CHECK(res.m_verify[4] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 5);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[4] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 2 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_verify.size() == 5);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashC);
        BOOST_CHECK(res.m_verify[2] == hashD);
        BOOST_CHECK(res.m_verify[3] == hashE);
        BOOST_CHECK(res.m_verify[4] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 5);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[4] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 3 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 4);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB);
        BOOST_CHECK(res.m_verify.size() == 4);
        BOOST_CHECK(res.m_verify[0] == hashC);
        BOOST_CHECK(res.m_verify[1] == hashD);
        BOOST_CHECK(res.m_verify[2] == hashE);
        BOOST_CHECK(res.m_verify[3] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 4);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 4 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashC);
        BOOST_CHECK(res.m_verify.size() == 5);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashD);
        BOOST_CHECK(res.m_verify[3] == hashE);
        BOOST_CHECK(res.m_verify[4] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 5);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[4] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 5 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_verify.size() == 4);
        BOOST_CHECK(res.m_verify[0] == hashB);
        BOOST_CHECK(res.m_verify[1] == hashD);
        BOOST_CHECK(res.m_verify[2] == hashE);
        BOOST_CHECK(res.m_verify[3] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 4);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 6 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_verify.size() == 4);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashD);
        BOOST_CHECK(res.m_verify[2] == hashE);
        BOOST_CHECK(res.m_verify[3] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 4);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 7 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C);
        BOOST_CHECK(res.m_verify.size() == 3);
        BOOST_CHECK(res.m_verify[0] == hashD);
        BOOST_CHECK(res.m_verify[1] == hashE);
        BOOST_CHECK(res.m_verify[2] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 3);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 8 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD);
        BOOST_CHECK(res.m_verify.size() == 5);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        BOOST_CHECK(res.m_verify[3] == hashE);
        BOOST_CHECK(res.m_verify[4] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 5);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[4] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 15 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashE);
        BOOST_CHECK(res.m_verify[1] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 16 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashE);
        BOOST_CHECK(res.m_verify.size() == 5);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        BOOST_CHECK(res.m_verify[3] == hashD);
        BOOST_CHECK(res.m_verify[4] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 5);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[4] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 23 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashE);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashD);
        BOOST_CHECK(res.m_verify[1] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 24 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashE);
        BOOST_CHECK(res.m_verify.size() == 4);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        BOOST_CHECK(res.m_verify[3] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 4);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 31 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 3);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD);
        BOOST_CHECK(res.m_proof.m_skip[2] == hashE);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashF);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[5]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 32 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashF);
        BOOST_CHECK(res.m_verify.size() == 5);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        BOOST_CHECK(res.m_verify[3] == hashD);
        BOOST_CHECK(res.m_verify[4] == hashE);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 5);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[4] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 39 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashF);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashD);
        BOOST_CHECK(res.m_verify[1] == hashE);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 40 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 5);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[4] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashF);
        BOOST_CHECK(res.m_verify.size() == 4);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        BOOST_CHECK(res.m_verify[3] == hashE);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 4);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 47 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 3);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD);
        BOOST_CHECK(res.m_proof.m_skip[2] == hashF);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashE);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[4]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 48 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 4);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[3] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashEF);
        BOOST_CHECK(res.m_verify.size() == 4);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        BOOST_CHECK(res.m_verify[3] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 4);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(branches[3] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 55 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::DESCEND));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashEF);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashD);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[3]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 56 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashD_EF);
        BOOST_CHECK(res.m_verify.size() == 3);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        BOOST_CHECK(res.m_verify[2] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 3);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[2] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 57 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD_EF);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashB);
        BOOST_CHECK(res.m_verify[1] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 58 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD_EF);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 59 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 2);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD_EF);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashC);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[2]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 60 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 2);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashC);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashD_EF);
        BOOST_CHECK(res.m_verify.size() == 2);
        BOOST_CHECK(res.m_verify[0] == hashA);
        BOOST_CHECK(res.m_verify[1] == hashB);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 2);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(branches[1] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 61 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::SKIP,
                        MerkleLink::VERIFY));
        BOOST_CHECK(res.m_proof.m_skip.size() == 3);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashA);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_proof.m_skip[2] == hashD_EF);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashB);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[1]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 62 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.size() == 3);
        BOOST_CHECK(res.m_proof.m_path[0] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[1] == MerkleNode(
                        MerkleLink::DESCEND,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_path[2] == MerkleNode(
                        MerkleLink::VERIFY,
                        MerkleLink::SKIP));
        BOOST_CHECK(res.m_proof.m_skip.size() == 3);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashB);
        BOOST_CHECK(res.m_proof.m_skip[1] == hashC);
        BOOST_CHECK(res.m_proof.m_skip[2] == hashD_EF);
        BOOST_CHECK(res.m_verify.size() == 1);
        BOOST_CHECK(res.m_verify[0] == hashA);
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(branches.size(), 1);
        BOOST_CHECK(branches[0] == branchesAB_C__D_EF[0]);
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }

    {
        /* 63 */
        auto res = MerkleTree(
            MerkleTree(
                MerkleTree(
                    MerkleTree(hashA, false),
                    MerkleTree(hashB, false)),
                MerkleTree(hashC, false)),
            MerkleTree(
                MerkleTree(hashD, false),
                MerkleTree(
                    MerkleTree(hashE, false),
                    MerkleTree(hashF, false))));
        BOOST_CHECK(res.m_proof.m_path.empty());
        BOOST_CHECK(res.m_proof.m_skip.size() == 1);
        BOOST_CHECK(res.m_proof.m_skip[0] == hashAB_C__D_EF);
        BOOST_CHECK(res.m_verify.empty());
        invalid = true;
        branches.clear();
        BOOST_CHECK(res.GetHash(&invalid, &branches) == hashAB_C__D_EF);
        BOOST_CHECK(!invalid);
        BOOST_CHECK(branches.empty());
        BOOST_CHECK(MerkleTree(zero, res) == res);
        BOOST_CHECK(MerkleTree(res, zero) == res);
    }
}

BOOST_AUTO_TEST_CASE(fast_merkle_branch)
{
    using std::swap;
    const std::vector<uint256> leaves = {
      (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('a')).GetHash(),
      (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('b')).GetHash(),
      (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('c')).GetHash(),
    };
    const uint256 root = ComputeFastMerkleRoot(leaves);
    BOOST_CHECK(root == uint256S("0x35d7dea3df173ecb85f59ebb88b2003be3c94b576576b12eb8d017f9fc33b289"));
    {
        std::vector<uint256> branch;
        uint32_t path;
        std::tie(branch, path) = ComputeFastMerkleBranch(leaves, 0);
        BOOST_CHECK(path == 0);
        BOOST_CHECK(branch.size() == 2);
        BOOST_CHECK(branch[0] == leaves[1]);
        BOOST_CHECK(branch[1] == leaves[2]);
        BOOST_CHECK(root == ComputeFastMerkleRootFromBranch(leaves[0], branch, path));
    }
    {
        std::vector<uint256> branch;
        uint32_t path;
        std::tie(branch, path) = ComputeFastMerkleBranch(leaves, 1);
        BOOST_CHECK(path == 1);
        BOOST_CHECK(branch.size() == 2);
        BOOST_CHECK(branch[0] == leaves[0]);
        BOOST_CHECK(branch[1] == leaves[2]);
        BOOST_CHECK(root == ComputeFastMerkleRootFromBranch(leaves[1], branch, path));
    }
    {
        std::vector<uint256> branch;
        uint32_t path;
        std::tie(branch, path) = ComputeFastMerkleBranch(leaves, 2);
        BOOST_CHECK(path == 1);
        BOOST_CHECK(branch.size() == 1);
        BOOST_CHECK(branch[0] == uint256S("0xa6e8f6cfa607807d35da463f0599aa0d8032dda4e5635c806098a9ed332b6279"));
        BOOST_CHECK(root == ComputeFastMerkleRootFromBranch(leaves[2], branch, path));
    }
    BOOST_CHECK(ComputeFastMerkleRoot({}) == MerkleTree().GetHash());
    for (int i = 1; i < 35; ++i) {
        std::vector<uint256> leaves;
        leaves.resize(i);
        for (int j = 0; j < i; ++j) {
            leaves.back().begin()[j/8] ^= ((char)1 << (j%8));
        }
        const uint256 root = ComputeFastMerkleRoot(leaves);
        for (int j = 0; j < i; ++j) {
            const auto branch = ComputeFastMerkleBranch(leaves, j);
            BOOST_CHECK(ComputeFastMerkleRootFromBranch(leaves[j], branch.first, branch.second) == root);
            std::vector<MerkleTree> subtrees(i);
            for (int k = 0; k < i; ++k) {
                if (k == j) {
                    subtrees[k].m_verify.push_back(leaves[k]);
                } else {
                    subtrees[k].m_proof.m_skip.push_back(leaves[k]);
                }
            }
            while (subtrees.size() > 1) {
                std::vector<MerkleTree> other;
                for (auto itr = subtrees.begin(); itr != subtrees.end(); ++itr) {
                    auto itr2 = std::next(itr);
                    if (itr2 != subtrees.end()) {
                        other.emplace_back(*(itr++), *itr2);
                    } else {
                        other.emplace_back();
                        swap(other.back(), *itr);
                    }
                }
                swap(subtrees, other);
            }
            BOOST_CHECK(subtrees[0].m_verify.size() == 1);
            bool invalid = false;
            std::vector<MerkleBranch> branches;
            BOOST_CHECK(subtrees[0].GetHash(&invalid, &branches) == root);
            BOOST_CHECK(!invalid);
            BOOST_CHECK(branches.size() == 1);
            BOOST_CHECK(branches[0].m_branch == branch.first);
            BOOST_CHECK(branches[0].GetPath() == branch.second);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
