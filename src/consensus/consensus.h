// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
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

#ifndef FREICOIN_CONSENSUS_CONSENSUS_H
#define FREICOIN_CONSENSUS_CONSENSUS_H

#include <stdint.h>

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 4000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
static const unsigned int MAX_BLOCK_WEIGHT = 4000000;
/** The maximum allowed size for a block excluding witness and auxiliary
 ** proof-of-work data, in bytes (network rule) */
static const unsigned int MAX_BLOCK_BASE_SIZE = 1000000;
/** The maximum size of a blk?????.dat file (since v10)
 ** (post-cleanup network rule) */
static const unsigned int PROTOCOL_CLEANUP_MAX_BLOCKFILE_SIZE = 0x7f000000; // (2048 - 16) MiB
/** The maximum serialized block size is constrained by the need to
 ** fit inside a block file, which has an additional 8 bytes of file
 ** data per block. (post-cleanup consensus rule) */
static const unsigned int PROTOCOL_CLEANUP_MAX_BLOCK_SERIALIZED_SIZE = 0x7efffff8; // PROTOCOL_CLEANUP_MAX_BLOCKFILE_SIZE - 8
/** The actual hard block size limit post-activation of the protocol
 ** cleanup rules is PROTOCOL_CLEANUP_MAX_BLOCKFILE_SIZE - 8, but this
 ** limit can't be reached for any real block because classic blocks
 ** have a minimum size and non-witness data has quadruple weight.  So
 ** we can still keep the post-fork weight limit as a relatively round
 ** number in binary, for faster calculations.  (post-cleanup
 ** consensus rule) */
static const unsigned int PROTOCOL_CLEANUP_MAX_BLOCK_WEIGHT = 0x7f000000; // (2048 - 16) MiB
/** The maximum block base block size is 1/4 the maximum block weight.
 ** (post-cleanup consensus rule) */
static const unsigned int PROTOCOL_CLEANUP_MAX_BLOCK_BASE_SIZE = 0x1fc00000; // 508 MiB
/** The maximum allowed number of signature check operations in a block (network rule) */
static const int64_t MAX_BLOCK_SIGOPS_COST = 80000;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

#endif // FREICOIN_CONSENSUS_CONSENSUS_H
