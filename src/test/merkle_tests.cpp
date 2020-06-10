// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_CASE(fast_merkle_branch)
{
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
}

BOOST_AUTO_TEST_SUITE_END()
