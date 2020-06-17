// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef FREICOIN_CONSENSUS_CONSENSUS_H
#define FREICOIN_CONSENSUS_CONSENSUS_H

#include <cstdlib>
#include <stdint.h>

/** The maximum number of hashes allowed in the path from the auxiliary block
 ** header to auxiliary block-final transaction.  Sufficient to support 2GB
 ** blocks on the auxiliary block chain. */
static const unsigned int MAX_AUX_POW_BRANCH_LENGTH = 25;
/** The maximum number of hashes allowed in the Merkle map proof within the
 ** auxiliary chain commitment.  A somewhat arbitrary number, chosen such that
 ** the maximum length path when serialized is about 1kB. */
static const unsigned int MAX_AUX_POW_COMMIT_BRANCH_LENGTH = 32;
/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 4000000;
/** The maximum allowed weight for a block (network rule) */
static const unsigned int MAX_BLOCK_WEIGHT = 4000000;
/** The maximum size of a blk?????.dat file (since v10)
 ** (post-expansion network rule) */
static const unsigned int SIZE_EXPANSION_MAX_BLOCKFILE_SIZE = 0x7f000000; // (2048 - 16) MiB
/** The maximum serialized block size is constrained by the need to
 ** fit inside a block file, which has an additional 8 bytes of file
 ** data per block. (post-expansion consensus rule) */
static const unsigned int SIZE_EXPANSION_MAX_BLOCK_SERIALIZED_SIZE = 0x7efffff8; // SIZE_EXPANSION_MAX_BLOCKFILE_SIZE - 8
/** The actual hard block size limit post-activation of the size expansion
 ** rules is SIZE_EXPANSION_MAX_BLOCKFILE_SIZE - 8, but this limit can't be
 ** reached for any real block because classic blocks have a minimum size and
 ** non-witness data has quadruple weight.  So we can still keep the post-fork
 ** weight limit as a relatively round number in binary, for faster
 ** calculations.  (post-expansion consensus rule) */
static const unsigned int SIZE_EXPANSION_MAX_BLOCK_WEIGHT = 0x7f000000; // (2048 - 16) MiB
/** The maximum allowed number of signature check operations in a block (network rule) */
static const int64_t MAX_BLOCK_SIGOPS_COST = 80000;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

static const int WITNESS_SCALE_FACTOR = 4;

static const size_t MIN_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 60; // 60 is the lower bound for the size of a valid serialized CTransaction
static const size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10; // 10 is the lower bound for the size of a serialized CTransaction

/** Flags for nSequence and nLockTime locks */
/** Interpret sequence numbers as relative lock-time constraints. */
static constexpr unsigned int LOCKTIME_VERIFY_SEQUENCE = (1 << 0);

#endif // FREICOIN_CONSENSUS_CONSENSUS_H
