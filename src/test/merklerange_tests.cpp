// Copyright (c) 2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('a')).GetHash(), // 0
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('b')).GetHash(), // 1
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('c')).GetHash(), // 2
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('d')).GetHash(), // 3
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('e')).GetHash(), // 4
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('f')).GetHash(), // 5
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('g')).GetHash(), // 6
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('h')).GetHash(), // 7
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('i')).GetHash(), // 8
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('j')).GetHash(), // 9
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('k')).GetHash(), // 10
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('l')).GetHash(), // 11
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('m')).GetHash(), // 12
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('n')).GetHash(), // 13
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('o')).GetHash(), // 14
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('p')).GetHash(), // 15
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('q')).GetHash(), // 16
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
    hash_ab = MerkleHash_Sha256Midstate(leaves[0], leaves[1]);
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
    hash_abc = MerkleHash_Sha256Midstate(hash_ab, leaves[2]);
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
    hash_cd = MerkleHash_Sha256Midstate(leaves[2], leaves[3]);
    uint256 hash_abcd;
    hash_abcd = MerkleHash_Sha256Midstate(hash_ab, hash_cd);
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
    hash_abcde = MerkleHash_Sha256Midstate(hash_abcd, leaves[4]);
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
    hash_ef = MerkleHash_Sha256Midstate(leaves[4], leaves[5]);
    uint256 hash_abcdef;
    hash_abcdef = MerkleHash_Sha256Midstate(hash_abcd, hash_ef);
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
    hash_efg = MerkleHash_Sha256Midstate(hash_ef, leaves[6]);
    uint256 hash_abcdefg;
    hash_abcdefg = MerkleHash_Sha256Midstate(hash_abcd, hash_efg);
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
    hash_gh = MerkleHash_Sha256Midstate(leaves[6], leaves[7]);
    uint256 hash_efgh;
    hash_efgh = MerkleHash_Sha256Midstate(hash_ef, hash_gh);
    uint256 hash_abcdefgh;
    hash_abcdefgh = MerkleHash_Sha256Midstate(hash_abcd, hash_efgh);
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
    hash_abcdefghi = MerkleHash_Sha256Midstate(hash_abcdefgh, leaves[8]);
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
    hash_ij = MerkleHash_Sha256Midstate(leaves[8], leaves[9]);
    uint256 hash_abcdefghij;
    hash_abcdefghij = MerkleHash_Sha256Midstate(hash_abcdefgh, hash_ij);
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
    hash_ijk = MerkleHash_Sha256Midstate(hash_ij, leaves[10]);
    uint256 hash_abcdefghijk;
    hash_abcdefghijk = MerkleHash_Sha256Midstate(hash_abcdefgh, hash_ijk);
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
    hash_kl = MerkleHash_Sha256Midstate(leaves[10], leaves[11]);
    uint256 hash_ijkl;
    hash_ijkl = MerkleHash_Sha256Midstate(hash_ij, hash_kl);
    uint256 hash_abcdefghijkl;
    hash_abcdefghijkl = MerkleHash_Sha256Midstate(hash_abcdefgh, hash_ijkl);
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
    hash_ijklm = MerkleHash_Sha256Midstate(hash_ijkl, leaves[12]);
    uint256 hash_abcdefghijklm;
    hash_abcdefghijklm = MerkleHash_Sha256Midstate(hash_abcdefgh, hash_ijklm);
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
    hash_mn = MerkleHash_Sha256Midstate(leaves[12], leaves[13]);
    uint256 hash_ijklmn;
    hash_ijklmn = MerkleHash_Sha256Midstate(hash_ijkl, hash_mn);
    uint256 hash_abcdefghijklmn;
    hash_abcdefghijklmn = MerkleHash_Sha256Midstate(hash_abcdefgh, hash_ijklmn);
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
    hash_mno = MerkleHash_Sha256Midstate(hash_mn, leaves[14]);
    uint256 hash_ijklmno;
    hash_ijklmno = MerkleHash_Sha256Midstate(hash_ijkl, hash_mno);
    uint256 hash_abcdefghijklmno;
    hash_abcdefghijklmno = MerkleHash_Sha256Midstate(hash_abcdefgh, hash_ijklmno);
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
    hash_op = MerkleHash_Sha256Midstate(leaves[14], leaves[15]);
    uint256 hash_mnop;
    hash_mnop = MerkleHash_Sha256Midstate(hash_mn, hash_op);
    uint256 hash_ijklmnop;
    hash_ijklmnop = MerkleHash_Sha256Midstate(hash_ijkl, hash_mnop);
    uint256 hash_abcdefghijklmnop;
    hash_abcdefghijklmnop = MerkleHash_Sha256Midstate(hash_abcdefgh, hash_ijklmnop);
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
    hash_abcdefghijklmnopq = MerkleHash_Sha256Midstate(hash_abcdefghijklmnop, leaves[16]);
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
        (CHashWriter(PROTOCOL_VERSION) << static_cast<uint8_t>('a')).GetHash(), // 0
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
