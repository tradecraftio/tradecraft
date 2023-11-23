// Copyright (c) 2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merklerange.h>

#include <consensus/merkle.h> // for MerkleHash_Sha256Midstate
#include <crypto/sha256.h> // for CSHA256
#include <uint256.h> // for uint256

MmrAccumulator& MmrAccumulator::Append(const uint256& leaf) {
    // Add leaf as a new peak.
    peaks.push_back(leaf);
    if (leaf_count & 1) {
        // The new leaf will aggregate with at least one existing peak, and
        // will continue to aggregate for each level it bumps into an existing
        // peak as it moves up the list of peaks.
        for (size_t i = 1; leaf_count & i; i <<= 1) {
            peaks[peaks.size() - 2] = MerkleHash_Sha256Midstate(peaks[peaks.size() - 2], peaks[peaks.size() - 1]);
            peaks.pop_back();
        }
    }
    // Increment the leaf count.
    ++leaf_count;
    // Support chaining.
    return *this;
}

uint256 MmrAccumulator::GetHash() const {
    // If there are no leaves, the aggregate hash is the null hash.
    if (leaf_count == 0) {
        return uint256();
    }
    // "Bag the peaks," accumulating from right to left.
    uint256 hash = peaks.back();
    for (auto it = peaks.rbegin() + 1; it != peaks.rend(); ++it) {
        hash = MerkleHash_Sha256Midstate(*it, hash);
    }
    return hash;
}

void swap(MmrAccumulator& lhs, MmrAccumulator& rhs) {
    using std::swap; // for ADL
    swap(lhs.leaf_count, rhs.leaf_count);
    swap(lhs.peaks, rhs.peaks);
}

// End of File
