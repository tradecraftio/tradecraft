// Copyright (c) 2023 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <consensus/merklerange.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <consensus/merkle.h> // for MerkleHash_Sha256Midstate

BOOST_FIXTURE_TEST_SUITE(merklerange_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(merklerange_empty) {
    MmrAccumulator mmr;
    BOOST_CHECK_EQUAL(mmr.leaf_count, 0);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 0);
    BOOST_CHECK_EQUAL(mmr.empty(), true);
    BOOST_CHECK_EQUAL(mmr.size(), 0);
    BOOST_CHECK_EQUAL(mmr.GetHash(), uint256{}); // null hash
}

BOOST_AUTO_TEST_CASE(merklerange_append) {
    const std::vector<uint256> leaves = {
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('a')).GetHash(), // 0
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('b')).GetHash(), // 1
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('c')).GetHash(), // 2
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('d')).GetHash(), // 3
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('e')).GetHash(), // 4
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('f')).GetHash(), // 5
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('g')).GetHash(), // 6
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('h')).GetHash(), // 7
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('i')).GetHash(), // 8
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('j')).GetHash(), // 9
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('k')).GetHash(), // 10
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('l')).GetHash(), // 11
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('m')).GetHash(), // 12
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('n')).GetHash(), // 13
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('o')).GetHash(), // 14
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('p')).GetHash(), // 15
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('q')).GetHash(), // 16
    };

    // <empty>
    MmrAccumulator mmr;
    BOOST_CHECK_EQUAL(mmr.empty(), true);

    // a
    mmr.Append(leaves[0]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 1);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 1);
    BOOST_CHECK_EQUAL(mmr.peaks[0], leaves[0]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), leaves[0]); // pass through

    //  ab
    // /  \
    // a   b
    uint256 hash_ab;
    MerkleHash_Sha256Midstate(hash_ab, leaves[0], leaves[1]);
    mmr.Append(leaves[1]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 2);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 1);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_ab);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_ab);

    //  ab
    // /  \
    // a   b  c
    uint256 hash_abc;
    MerkleHash_Sha256Midstate(hash_abc, hash_ab, leaves[2]);
    mmr.Append(leaves[2]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 3);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 2);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_ab);
    BOOST_CHECK_EQUAL(mmr.peaks[1], leaves[2]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abc);

    //     abcd
    //    /    \
    //  ab     cd
    // /  \   /  \
    // a   b  c   d
    uint256 hash_cd;
    MerkleHash_Sha256Midstate(hash_cd, leaves[2], leaves[3]);
    uint256 hash_abcd;
    MerkleHash_Sha256Midstate(hash_abcd, hash_ab, hash_cd);
    mmr.Append(leaves[3]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 4);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 1);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcd);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcd);

    //     abcd
    //    /    \
    //  ab     cd
    // /  \   /  \
    // a   b  c   d  e
    uint256 hash_abcde;
    MerkleHash_Sha256Midstate(hash_abcde, hash_abcd, leaves[4]);
    mmr.Append(leaves[4]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 5);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 2);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcd);
    BOOST_CHECK_EQUAL(mmr.peaks[1], leaves[4]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcde);

    //     abcd
    //    /    \
    //  ab     cd     ef
    // /  \   /  \   /  \
    // a   b  c   d  e   f
    uint256 hash_ef;
    MerkleHash_Sha256Midstate(hash_ef, leaves[4], leaves[5]);
    uint256 hash_abcdef;
    MerkleHash_Sha256Midstate(hash_abcdef, hash_abcd, hash_ef);
    mmr.Append(leaves[5]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 6);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 2);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcd);
    BOOST_CHECK_EQUAL(mmr.peaks[1], hash_ef);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdef);

    //     abcd
    //    /    \
    //  ab     cd     ef
    // /  \   /  \   /  \
    // a   b  c   d  e   f  g
    uint256 hash_efg;
    MerkleHash_Sha256Midstate(hash_efg, hash_ef, leaves[6]);
    uint256 hash_abcdefg;
    MerkleHash_Sha256Midstate(hash_abcdefg, hash_abcd, hash_efg);
    mmr.Append(leaves[6]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 7);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 3);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcd);
    BOOST_CHECK_EQUAL(mmr.peaks[1], hash_ef);
    BOOST_CHECK_EQUAL(mmr.peaks[2], leaves[6]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefg);

    //          abcdefgh
    //         /        \
    //     abcd          efgh
    //    /    \        /    \
    //  ab     cd     ef     gh
    // /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h
    uint256 hash_gh;
    MerkleHash_Sha256Midstate(hash_gh, leaves[6], leaves[7]);
    uint256 hash_efgh;
    MerkleHash_Sha256Midstate(hash_efgh, hash_ef, hash_gh);
    uint256 hash_abcdefgh;
    MerkleHash_Sha256Midstate(hash_abcdefgh, hash_abcd, hash_efgh);
    mmr.Append(leaves[7]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 8);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 1);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefgh);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefgh);

    //          abcdefgh
    //         /        \
    //     abcd          efgh
    //    /    \        /    \
    //  ab     cd     ef     gh
    // /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i
    uint256 hash_abcdefghi;
    MerkleHash_Sha256Midstate(hash_abcdefghi, hash_abcdefgh, leaves[8]);
    mmr.Append(leaves[8]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 9);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 2);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefgh);
    BOOST_CHECK_EQUAL(mmr.peaks[1], leaves[8]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghi);

    //          abcdefgh
    //         /        \
    //     abcd          efgh
    //    /    \        /    \
    //  ab     cd     ef     gh     ij
    // /  \   /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i   j
    uint256 hash_ij;
    MerkleHash_Sha256Midstate(hash_ij, leaves[8], leaves[9]);
    uint256 hash_abcdefghij;
    MerkleHash_Sha256Midstate(hash_abcdefghij, hash_abcdefgh, hash_ij);
    mmr.Append(leaves[9]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 10);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 2);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefgh);
    BOOST_CHECK_EQUAL(mmr.peaks[1], hash_ij);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghij);

    //          abcdefgh
    //         /        \
    //     abcd          efgh
    //    /    \        /    \
    //  ab     cd     ef     gh     ij
    // /  \   /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i   j  k
    uint256 hash_ijk;
    MerkleHash_Sha256Midstate(hash_ijk, hash_ij, leaves[10]);
    uint256 hash_abcdefghijk;
    MerkleHash_Sha256Midstate(hash_abcdefghijk, hash_abcdefgh, hash_ijk);
    mmr.Append(leaves[10]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 11);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 3);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefgh);
    BOOST_CHECK_EQUAL(mmr.peaks[1], hash_ij);
    BOOST_CHECK_EQUAL(mmr.peaks[2], leaves[10]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghijk);

    //          abcdefgh
    //         /        \
    //     abcd          efgh          ijkl
    //    /    \        /    \        /    \
    //  ab     cd     ef     gh     ij     kl
    // /  \   /  \   /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i   j  k   l
    uint256 hash_kl;
    MerkleHash_Sha256Midstate(hash_kl, leaves[10], leaves[11]);
    uint256 hash_ijkl;
    MerkleHash_Sha256Midstate(hash_ijkl, hash_ij, hash_kl);
    uint256 hash_abcdefghijkl;
    MerkleHash_Sha256Midstate(hash_abcdefghijkl, hash_abcdefgh, hash_ijkl);
    mmr.Append(leaves[11]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 12);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 2);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefgh);
    BOOST_CHECK_EQUAL(mmr.peaks[1], hash_ijkl);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghijkl);

    //          abcdefgh
    //         /        \
    //     abcd          efgh          ijkl
    //    /    \        /    \        /    \
    //  ab     cd     ef     gh     ij     kl
    // /  \   /  \   /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i   j  k   l  m
    uint256 hash_ijklm;
    MerkleHash_Sha256Midstate(hash_ijklm, hash_ijkl, leaves[12]);
    uint256 hash_abcdefghijklm;
    MerkleHash_Sha256Midstate(hash_abcdefghijklm, hash_abcdefgh, hash_ijklm);
    mmr.Append(leaves[12]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 13);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 3);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefgh);
    BOOST_CHECK_EQUAL(mmr.peaks[1], hash_ijkl);
    BOOST_CHECK_EQUAL(mmr.peaks[2], leaves[12]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghijklm);

    //          abcdefgh
    //         /        \
    //     abcd          efgh          ijkl
    //    /    \        /    \        /    \
    //  ab     cd     ef     gh     ij     kl     mn
    // /  \   /  \   /  \   /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i   j  k   l  m   n
    uint256 hash_mn;
    MerkleHash_Sha256Midstate(hash_mn, leaves[12], leaves[13]);
    uint256 hash_ijklmn;
    MerkleHash_Sha256Midstate(hash_ijklmn, hash_ijkl, hash_mn);
    uint256 hash_abcdefghijklmn;
    MerkleHash_Sha256Midstate(hash_abcdefghijklmn, hash_abcdefgh, hash_ijklmn);
    mmr.Append(leaves[13]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 14);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 3);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefgh);
    BOOST_CHECK_EQUAL(mmr.peaks[1], hash_ijkl);
    BOOST_CHECK_EQUAL(mmr.peaks[2], hash_mn);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghijklmn);

    //          abcdefgh
    //         /        \
    //     abcd          efgh          ijkl
    //    /    \        /    \        /    \
    //  ab     cd     ef     gh     ij     kl     mn
    // /  \   /  \   /  \   /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i   j  k   l  m   n  o
    uint256 hash_mno;
    MerkleHash_Sha256Midstate(hash_mno, hash_mn, leaves[14]);
    uint256 hash_ijklmno;
    MerkleHash_Sha256Midstate(hash_ijklmno, hash_ijkl, hash_mno);
    uint256 hash_abcdefghijklmno;
    MerkleHash_Sha256Midstate(hash_abcdefghijklmno, hash_abcdefgh, hash_ijklmno);
    mmr.Append(leaves[14]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 15);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 4);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefgh);
    BOOST_CHECK_EQUAL(mmr.peaks[1], hash_ijkl);
    BOOST_CHECK_EQUAL(mmr.peaks[2], hash_mn);
    BOOST_CHECK_EQUAL(mmr.peaks[3], leaves[14]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghijklmno);

    //                    abcdefghijklmnop
    //                  /                  \
    //          abcdefgh                    ijklmnop
    //         /        \                  /        \
    //     abcd          efgh          ijkl          mnop
    //    /    \        /    \        /    \        /    \
    //  ab     cd     ef     gh     ij     kl     mn     op
    // /  \   /  \   /  \   /  \   /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i   j  k   l  m   n  o   p
    uint256 hash_op;
    MerkleHash_Sha256Midstate(hash_op, leaves[14], leaves[15]);
    uint256 hash_mnop;
    MerkleHash_Sha256Midstate(hash_mnop, hash_mn, hash_op);
    uint256 hash_ijklmnop;
    MerkleHash_Sha256Midstate(hash_ijklmnop, hash_ijkl, hash_mnop);
    uint256 hash_abcdefghijklmnop;
    MerkleHash_Sha256Midstate(hash_abcdefghijklmnop, hash_abcdefgh, hash_ijklmnop);
    mmr.Append(leaves[15]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 16);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 1);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefghijklmnop);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghijklmnop);

    //                    abcdefghijklmnop
    //                  /                  \
    //          abcdefgh                    ijklmnop
    //         /        \                  /        \
    //     abcd          efgh          ijkl          mnop
    //    /    \        /    \        /    \        /    \
    //  ab     cd     ef     gh     ij     kl     mn     op
    // /  \   /  \   /  \   /  \   /  \   /  \   /  \   /  \
    // a   b  c   d  e   f  g   h  i   j  k   l  m   n  o   p  q
    uint256 hash_abcdefghijklmnopq;
    MerkleHash_Sha256Midstate(hash_abcdefghijklmnopq, hash_abcdefghijklmnop, leaves[16]);
    mmr.Append(leaves[16]);
    BOOST_CHECK_EQUAL(mmr.empty(), false);
    BOOST_CHECK_EQUAL(mmr.size(), 17);
    BOOST_CHECK_EQUAL(mmr.peaks.size(), 2);
    BOOST_CHECK_EQUAL(mmr.peaks[0], hash_abcdefghijklmnop);
    BOOST_CHECK_EQUAL(mmr.peaks[1], leaves[16]);
    BOOST_CHECK_EQUAL(mmr.GetHash(), hash_abcdefghijklmnopq);
}

BOOST_AUTO_TEST_CASE(merklerange_swap) {
    const std::vector<uint256> leaves = {
        (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << static_cast<uint8_t>('a')).GetHash(), // 0
    };

    // <empty>
    MmrAccumulator mmr_empty;
    BOOST_CHECK_EQUAL(mmr_empty.empty(), true);

    // a
    MmrAccumulator mmr_a;
    mmr_a.Append(leaves[0]);
    BOOST_CHECK_EQUAL(mmr_a.empty(), false);
    BOOST_CHECK_EQUAL(mmr_a.size(), 1);
    BOOST_CHECK_EQUAL(mmr_a.peaks.size(), 1);
    BOOST_CHECK_EQUAL(mmr_a.peaks[0], leaves[0]);
    BOOST_CHECK_EQUAL(mmr_a.GetHash(), leaves[0]); // pass through

    using std::swap; // enable ADL

    swap(mmr_empty, mmr_a);

    BOOST_CHECK_EQUAL(mmr_empty.empty(), false);
    BOOST_CHECK_EQUAL(mmr_empty.size(), 1);
    BOOST_CHECK_EQUAL(mmr_empty.peaks.size(), 1);
    BOOST_CHECK_EQUAL(mmr_empty.peaks[0], leaves[0]);
    BOOST_CHECK_EQUAL(mmr_empty.GetHash(), leaves[0]); // pass through

    BOOST_CHECK_EQUAL(mmr_a.empty(), true);
}

BOOST_AUTO_TEST_SUITE_END()

// End of File
