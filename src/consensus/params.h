// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
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

#ifndef FREICOIN_CONSENSUS_PARAMS_H
#define FREICOIN_CONSENSUS_PARAMS_H

#include "uint256.h"

namespace Consensus {
/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int per_block_demurrage_factor;
    int64_t equilibrium_monetary_base;
    int equilibrium_height;
    int64_t initial_block_reward;
    int64_t alu_activation_height;
    int64_t verify_coinbase_lock_time_minimum_height;
    int64_t verify_coinbase_lock_time_activation_time;
    /** Used to check majorities for block version upgrade */
    int nMajorityEnforceBlockUpgrade;
    int nMajorityRejectBlockOutdated;
    int nMajorityWindow;
    int activate_bit_upgrade_majority;
    int to_check_bit_upgrade_majority;

    /**
     * Scheduled protocol cleanup rule change
     *
     * To achieve desired scaling limits, the forward blocks architecture
     * will eventually trigger a hard-fork modification of the consensus
     * rules, for the primary purpose of dropping enforcement of many
     * aggregate block limits and altering the difficulty adjustment
     * algorithm.
     *
     * This hard-fork will not activate until it is absolutely necessary
     * for it to do so, at the point when real demand for additional shard
     * space in aggregate across all forward block shard-chains exceeds
     * the available space in the compatibility chain. It is anticipated
     * that this will not occur until many, many years into the future,
     * when Freicoin/Tradecraft's usage exceeds even the levels of bitcoin
     * usage ca. 2018. However when it does eventually trigger, any node
     * enforcing the old rules will be left behind.
     *
     * Since the rule changes for forward blocks have not been written yet
     * and becauses this flag-day code doesn't know how to detect actual
     * activation, we cannot have older clients enforce the new rules.
     * What is done instead is that any rule which we anticipate changes
     * becomes simply unenforced after this activation time, and aggregate
     * limits are set to the maximum values the software is able to
     * support. After the flag-day, older clients of at least version 10.4
     * will continue to receive blocks, but with only SPV security ("trust
     * the most work") for the new protocol rules. So starting with the
     * release of v10.4-7842, activation of forward blocks new scaling
     * limits becomes a soft-fork, with the only concern being the forking
     * off of 0.9 series and earlier nodes upon activation.
     *
     * The primary rules which must be altered for forward blocks scaling
     * are:
     *
     *   1. Significant relaxation of the rules regarding per-block
     *      difficulty adjustment, to allow adjustments of +/- 2x within
     *      twelve blocks, without regard of a target interval. Forward
     *      blocks will have a new difficulty adjustment algorithm that
     *      has yet to be determined, and will include adjusting to a
     *      variable inter-block time to achieve compatability chain
     *      scalability.
     *
     *   2. Increase of the maximum block size. Uncapping the block size
     *      is not possible because even if the explicit limit is removed
     *      there are still implicit network and disk protocol limits that
     *      would prevent a client from syncing a chain with larger
     *      blocks. But these network and disk limits could be set much
     *      higher than the current limits based on a 1 megabyte
     *      MAX_BLOCK_SIZE.
     *
     *   3. Allow larger transactions, up to the new, larger maximum block
     *      size limit in size. This is less safe than increasing the
     *      block size since most of the quadratic validation costs are
     *      only quadratic in transaction size. But there is research to
     *      be done in choosing what new limits should be used, and in the
     *      mean time keeping transactions only limited by the (new) block
     *      size permits flexibility in that future choice.
     *
     * That is all that MUST be done, but there are a number of other rule
     * changes that are related, or are trivial to accomplish at the same
     * time. These include:
     *
     *   4. Removal of MAX_BLOCK_SIGOPS limit. Switching to libsecp256k1
     *      for validation (which is done upstream in Bitcoin Core 0.12)
     *      and better signature / script and transaction validation
     *      caching has made this limit nearly redundant.
     *
     *   5. Allow a transaction without transaction outputs. A transaction
     *      must have inputs to have a unique transactionn ID, but it need
     *      not have outputs. There are obscure cases when this makes
     *      sense to do and (thus forward the funds entirely as "fee" to
     *      the miner, to process in the coinbase in some way).
     *
     *   6. Do not restrict the contents of the "coinbase string" in any
     *      way. It is currently required to be between 2 and 100 bytes in
     *      size, and must begin with the serialized block height. The
     *      length restriction is unnecesary as miners have other means of
     *      padding transactions if they need to, and are generally
     *      incentivised not to because of miner fees. The serialized
     *      height requirement is redundant as lock_height is also
     *      required to be set to the current block height.
     *
     *   7. Reduce coinbase maturity to 1 block. Once forward blocks
     *      has activated, coinbase maturity is an unnecessary delay
     *      to processing the coinbase payout queue.
     *
     *   8. Do not require zero-valued outputs to be spent by transactions
     *      with lock_height >= the coin's refheight. This restriction
     *      ensured that refheights were always increasing so that
     *      demurrage is collected, not reversed. However this argument
     *      doesn't really make sense for zero-valued outputs. At the same
     *      time "zero-valued" outputs are increasingly likely to be used
     *      for confidential or non-freicoin assets using extension
     *      outputs, for which monotonic lock_heights are just an annoying
     *      protocol complication.
     *
     *   9. Do not reject "old" blocks after activation of the nVersion=2
     *      and nVersion=3 soft-forks. With the switch to version bits for
     *      soft-fork activation, this archaic check is shown to be rather
     *      pointless. Rules are enforced in a block if it is downstream
     *      of the point of activation, not based on the nVersion value.
     *      Implicitly this also allows "negative" block.nVersion values.
     *
     *   10. Lift restrictions inside the script interpreter on maximum
     *       script size, maximum data push, maximum number of elements
     *       on the stack, and maximum number of executed opcodes.
     *
     *   11. Remove checks on disabled opcodes, and cause unrecognized
     *       opcodes to "return true" instead of raising an error.
     *
     *   12. Re-enable (and implement) certain disabled opcodes, and
     *       conspiciously missing opcodes which were never there in the
     *       first place.
     *
     * These consensus rules are eliminated or significantly relaxed at
     * the same time as the aggregate limits are removed, hence the
     * general label a "protocol cleanup" fork.
     **/
    int64_t protocol_cleanup_activation_time;

    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    /** Difficulty adjustment parameters */
    int64_t nPowTargetSpacing;
    int original_adjust_interval;
    int filtered_adjust_interval;
    int diff_adjust_threshold;
    int64_t OriginalTargetTimespan() const { return original_adjust_interval * nPowTargetSpacing; }
    int64_t FilteredTargetTimespan() const { return filtered_adjust_interval * nPowTargetSpacing; }
};
} // namespace Consensus

#endif // FREICOIN_CONSENSUS_PARAMS_H
