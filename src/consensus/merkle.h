// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_MERKLE_H
#define BITCOIN_CONSENSUS_MERKLE_H

#include <stdint.h>
#include <vector>

#include <primitives/transaction.h>
#include <primitives/block.h>
#include <uint256.h>

void MerkleHash_Hash256(uint256& parent, const uint256& left, const uint256& right);
void MerkleHash_Sha256Midstate(uint256& parent, const uint256& left, const uint256& right);

uint256 ComputeMerkleRoot(std::vector<uint256> hashes, bool* mutated);
std::vector<uint256> ComputeMerkleBranch(const std::vector<uint256>& leaves, uint32_t position);
uint256 ComputeMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t position);

/*
 * Has similar API semantics, but produces Merkle roots and validates
 * branches 2.32x as fast as the Satoshi Merkle tree, and without the
 * mutation vulnerability.  ComputeFastMerkleBranch returns a pair
 * with the second element being the path used to validate the branch.
 * Cannot be substituted for the non-fast variants because the output
 * hash values are different.
 *
 * In order to avoid unnecessary hash operations, and to fix the
 * CVE-2012-2459 vulnerability the fast Merkle tree operations do not
 * produce balanced binary tree unless the number of leaves given is a
 * power of 2.  It produces instead a tree where the root node's left
 * branch is to the largest power-of-2 subtree that can be constructed
 * with the first 2^N elements, and the remainder to the right.  The
 * node pointed to by the right branch has the largest possible
 * power-of-2 subtree of the remaining elements to its left, and the
 * remainder to its right.  This continues recursively until there is
 * a single leaf element on the far-rightmost branch.
 *
 * Alternatively it might be helpful to think of how ComputeMerkleRoot
 * would contrast with ComputeFastMerkleRoot if the old Satoshi tree
 * hashing algorithm were used:
 *
 * ComputeMerkleRoot essentially compressed its leaves vector by
 * replacing adjacent entries with their combined hash, and recursing
 * until only a single element remained.  When the number of leaf or
 * inner node hashes in the vector was an odd number greater than one,
 * the final element was hashed with itself.  The result could be
 * viewed as a perfectly balanced binary tree with some of the final
 * elements repreated when the size is not a power of 2.
 *
 * ComputeFastMerkleRoot, on the other hand, would be implemented by
 * *not* duplicating the final element when the size of the list is
 * odd.  This causes the right-side of the tree to be unbalanced when
 * the number of leaves is not a power of 2.
 *
 * With ComputeMerkleBranch and ComputeMerkleRootFromBranch, the path
 * from the leaf to the root of the tree is given by the binary
 * encoding of the leaf's position: 0 for left and 1 for right,
 * starting from the least significant bit and working upwards until
 * you run out of hashes.
 *
 * With the fast Merkle tree variants the path used to validate a
 * branch is derived from but not necessarily the same as the binary
 * representation of the original position in the list.
 * ComputeFastMerkleBranch calculates the path by dropping high-order
 * zeros from the binary representation of the position until the path
 * is the same length or less as the number of Merkle branches taken
 * to reach the node.
 *
 * To understand why this works, consider a list of 303 elements from
 * which a fast Merkle tree is constructed, and we request the branch to
 * the 292nd element. The binary encoded positions of the last and
 * desired elements are as follows:
 *
 *   0b 1 0 0 1 0 1 1 1 0 # decimal 302 (zero-indexed)
 *
 *   0b 1 0 0 1 0 0 0 1 1 # decimal 291
 *
 * The root of the Merkle tree has a left branch that contains 2^8 = 256
 * elements, and a right branch that contains the remaining 47. The first
 * level of the right branch contains 2^5 = 32 nodes on the left side, and
 * the remaining 15 nodes on the right side. The next level contains 2^3 =
 * 8 nodes on the left, and the remaining 7 on the right. This pattern
 * repeats on the right hand side of the tree: each layer has the largest
 * remaining power of two on the left, and the residual on the right.
 *
 * Notice specifically that the sizes of the sub-trees correspnd to the
 * set bits in the zero-based index of the final element. For each 1 at,
 * index n, there is a branch with 2^n elements on the left and the
 * remaining amount on the right.
 *
 * So, for an element whose path traverse the right-side of the tree, the
 * intervening levels (e.g. 2^7 and 2^6) are missing. These correspond to
 * zeros in the binary expansion, and they are removed from the path
 * description. However once the path takes a left-turn into the tree (a
 * zero where a one is present in the expansion of the last element), the
 * sub-tree is full and no more 0's can be pruned out.
 *
 * So the path for element 292 becomes:
 *
 *     0b 1 - - 1 - 0 0 1 1 # decimal 291
 *
 *   = 0b 1 1 0 0 1 1
 *
 *   = 51
 *
 * This path value is calculated and returned alongside the vector of
 * hashes representing the branch in the return value of
 * ComputeFastMerkleRoot.
 *
 * This path value, and *not* the original position value, should be
 * used in later calls to ComputeFastMerkleRootFromBranch.
 */
uint256 ComputeFastMerkleRoot(const std::vector<uint256>& leaves);
std::pair<std::vector<uint256>, uint32_t> ComputeFastMerkleBranch(const std::vector<uint256>& leaves, uint32_t position);
uint256 ComputeFastMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t path, bool* invalid = nullptr);

/*
 * Compute the Merkle root of the transactions in a block.
 * *mutated is set to true if a duplicated subtree was found.
 */
uint256 BlockMerkleRoot(const CBlock& block, bool* mutated = nullptr);

/*
 * Compute the Merkle root of the witness transactions in a block.
 * *mutated is set to true if a duplicated subtree was found.
 */
uint256 BlockWitnessMerkleRoot(const CBlock& block, bool* mutated = nullptr);

/*
 * Compute the Merkle branch for the tree of transactions in a block, for a
 * given position.
 * This can be verified using ComputeMerkleRootFromBranch.
 */
std::vector<uint256> BlockMerkleBranch(const CBlock& block, uint32_t position);

#endif // BITCOIN_CONSENSUS_MERKLE_H
