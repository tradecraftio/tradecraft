// Copyright (c) 2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// A Merkle Mountain Range is a type of prunable binary Merkle tree that has
// the following properties:
//
// 1. The tree is append-only.  Once a node is added to the tree, it can be
//    modified, but not removed.  Nodes are only added to the tree from the
//    right.
//
// 2. Only the "peaks" of the tree, and the total number of existing node
//    commitments are required to append a new node.  This is known as the
//    Merkle Mountain Range commitment.  Appending a node to the tree results
//    in a new set of "peaks" and increases the total number of nodes by one
//    (the new leaf value) plus the number of added inner nodes.
//
// 3. With only the Merkle Mountain Range commitment (peaks + total number of
//    nodes), and the path through the tree to a leaf node, that leaf node can
//    be mutated (but not deleted), resulting in a new Merkle Mountain Range
//    commitment.
//
// 4. An "observer" who maintains their own authentication proof for a
//    particular leaf node can maintain/update that proof when given a log of
//    the above operations: appending new nodes or modifying an existing node.
//
// 5. The Merkle Mountain Range commitment can be further reduced to a single
//    32-byte hash value, the Merkle Mountain Range root hash, by a process
//    called "bagging the peaks."  In this way a single hash value can be used
//    to commit to the state of the entire Merkle Mountain Range structure.
//
// The Merkle Mountain Range was originally described[1] by Peter Todd in the
// context of the OpenTimestamps project, where it is used to store an
// append-only log of commitments.  The data structure was further developed
// by the Grin project, where it was similarly used as an append-only record
// of Mimblewimble commitments[2].  This implementation draws ideas from both
// sources, but is a complete reimplementation from scratch.
//
// Another good description of the Merkle Mountain Range is provided by the
// developers of the Neptune protocol[3].
//
// [1]: https://github.com/opentimestamps/opentimestamps-server/blob/master/doc/merkle-mountain-range.md
// [2]: https://github.com/mimblewimble/grin/blob/master/doc/mmr.md
// [3]: https://neptune.cash/learn/mmr/

// Note: many implementations of MMR use Blake2b as the hash function.  This
//       implementation uses SHA256, which is the same hash function used by
//       the rest of the Bitcoin codebase.  Blake2b is in principle a faster
//       hash function, but the factor that dominates in reality is that most
//       modern hardware comes with built-in SHA256 intrinsics, but Blake2b
//       must be implemented in software.

#ifndef BITCOIN_CONSENSUS_MERKLERANGE_H
#define BITCOIN_CONSENSUS_MERKLERANGE_H

#include <serialize.h> // for VARINT, Serialize, Unserialize
#include <stddef.h> // for size_t
#include <uint256.h> // for uint256

#include <vector> // for vector

struct MmrAccumulator {
    //! The number of leaf nodes in the tree.
    size_t leaf_count;
    //! The hash of the peaks.  The length of this vector is the number of set
    //! bits in the binary representation of `leaf_count`.
    std::vector<uint256> peaks;

    MmrAccumulator() : leaf_count(0) {}
    MmrAccumulator(size_t leaf_count, std::vector<uint256> peaks)
        : leaf_count(leaf_count), peaks(peaks) { ASSERT_IF_DEBUG(peaks.size() == __builtin_popcount(leaf_count)); }

    //! Returns true if the accumulator is empty.
    bool empty() const { return leaf_count == 0; }
    //! Returns the number of leaf nodes in the tree.
    size_t size() const { return leaf_count; }

    //! Add a new leaf node to the tree.
    MmrAccumulator& Append(const uint256& leaf);

    //! Returns the hash of the peaks.
    uint256 GetHash() const;

    template<typename Stream>
    void Serialize(Stream &s) const {
        // The varint format developed by Pieter Wuille is a more compact
        // encoding scheme, and is better for canonical encodings as there is
        // only one way to encode any number, unlike Satoshi's CompactSize
        // encoding format.
        ::Serialize(s, VARINT(leaf_count));
        // The algorithm by which the accumulator is updated ensures the
        // interesting side effect that the number of peaks will always be
        // equal to the number of set bits in leaf_count.  Therefore encoding
        // the size of the peaks vector is unnecessary.
        ASSERT_IF_DEBUG(peaks.size() == __builtin_popcount(leaf_count));
        for (const auto& hash : peaks) {
            ::Serialize(s, hash);
        }
    }

    template<typename Stream>
    void Unserialize(Stream &s) {
        ::Unserialize(s, VARINT(leaf_count));
        // The number of peaks is equal to the number of set bits in
        // leaf_count, so the size of the peaks array is inferred rather than
        // encoded directly.
        peaks.resize(__builtin_popcount(leaf_count));
        for (auto& hash : peaks) {
            ::Unserialize(s, hash);
        }
    }
};

/* Defined outside the class for argument-dpendent lookup. */
void swap(MmrAccumulator& lhs, MmrAccumulator& rhs);

#endif // BITCOIN_CONSENSUS_MERKLERANGE_H

// End of File
