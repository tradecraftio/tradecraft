// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#ifndef FREICOIN_CONSENSUS_PARAMS_H
#define FREICOIN_CONSENSUS_PARAMS_H

#include <consensus/amount.h>
#include <uint256.h>
#include <util/time.h>

#include <chrono>
#include <limits>
#include <map>
#include <vector>

namespace Consensus {

enum RuleSet : uint8_t {
    NONE = 0,
    PROTOCOL_CLEANUP = (1U << 0),
    SIZE_EXPANSION = (1U << 1),
};

inline RuleSet operator | (RuleSet lhs, RuleSet rhs) {
    using T = std::underlying_type_t<RuleSet>;
    return static_cast<RuleSet>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline RuleSet& operator |= (RuleSet& lhs, RuleSet rhs) {
    lhs = lhs | rhs;
    return lhs;
}

/**
 * A buried deployment is one where the height of the activation has been hardcoded into
 * the client implementation long after the consensus change has activated. See BIP 90.
 */
enum BuriedDeployment : int16_t {
    // buried deployments get negative values to avoid overlap with DeploymentPos
    DEPLOYMENT_HEIGHTINCB = std::numeric_limits<int16_t>::min(),
    DEPLOYMENT_DERSIG,
    DEPLOYMENT_LOCKTIME,
    DEPLOYMENT_SEGWIT,
    DEPLOYMENT_CLEANUP,
};
constexpr bool ValidDeployment(BuriedDeployment dep) { return dep <= DEPLOYMENT_CLEANUP; }

enum DeploymentPos : uint16_t {
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_FINALTX, // Deployment of block-final miner commitment transaction.
    DEPLOYMENT_AUXPOW, // Deployment of merge mining.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in deploymentinfo.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};
constexpr bool ValidDeployment(DeploymentPos dep) { return dep < MAX_VERSION_BITS_DEPLOYMENTS; }

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit{28};
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime{NEVER_ACTIVE};
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout{NEVER_ACTIVE};
    /** If lock in occurs, delay activation until at least this block
     *  height.  Note that activation will only occur on a retarget
     *  boundary.
     */
    int min_activation_height{0};

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;

    /** Special value for nStartTime indicating that the deployment is never active.
     *  This is useful for integrating the code changes for a new feature
     *  prior to deploying it on some or all networks. */
    static constexpr int64_t NEVER_ACTIVE = -2;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    uint256 aux_pow_path;
    /** Bitcoin unit test compatibility mode */
    bool bitcoin_mode;
    int nSubsidyHalvingInterval;
    /** Perpetual distribution via constant block reward */
    CAmount perpetual_subsidy; // equilibrium_monetary_base * demurrage rate
    /** Initial distribution via excess subsidy */
    int64_t equilibrium_height;
    int64_t equilibrium_monetary_base;
    CAmount initial_excess_subsidy;
    /** Soft-fork activations */
    int64_t truncate_inputs_activation_height;
    int64_t alu_activation_height;
    int64_t verify_coinbase_lock_time_activation_height;
    /**
     * Hashes of blocks that
     * - are known to be consensus valid, and
     * - buried in the chain, and
     * - fail if the default script verify flags are applied.
     */
    std::map<uint256, uint32_t> script_flag_exceptions;
    /** Block height at which BIP34 becomes active */
    int BIP34Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    /** Block height at which locktime restrictions (BIP68 and BIP113) become active */
    int LockTimeHeight;
    /** Block height at which Segwit (BIP141, BIP143 and BIP147) becomes active.
     * Note that segwit v0 script rules are enforced on all blocks except the
     * BIP 16 exception blocks. */
    int SegwitHeight;
    /** Block height at which the protocl cleanup rule changes become active */
    int CleanupHeight;
    /** Don't warn about unknown BIP 9 activations below this height.
     * This prevents us from warning about the locktime and segwit activations. */
    int MinBIP9WarningHeight;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Scheduled protocol cleanup rule change */
    int64_t protocol_cleanup_activation_time;
    /** Scheduled size expansion rule change */
    int64_t size_expansion_activation_time;
    /** Proof of work parameters */
    uint256 powLimit;
    uint256 aux_pow_limit;
    bool fPowNoRetargeting;
    /** Difficulty adjustment parameters */
    int64_t nPowTargetSpacing;
    std::chrono::seconds PowTargetSpacing() const
    {
        return std::chrono::seconds{nPowTargetSpacing};
    }
    int64_t aux_pow_target_spacing;
    std::chrono::seconds AuxPowTargetSpacing() const
    {
        return std::chrono::seconds{aux_pow_target_spacing};
    }
    int64_t original_adjust_interval;
    int64_t filtered_adjust_interval;
    int64_t diff_adjust_threshold;
    int64_t OriginalTargetTimespan() const
    {
        return original_adjust_interval * nPowTargetSpacing;
    }
    int64_t FilteredTargetTimespan() const
    {
        return filtered_adjust_interval * nPowTargetSpacing;
    }
    /** The best chain should have at least this much work */
    uint256 nMinimumChainWork;
    /** By default assume that the signatures in ancestors of this block are valid */
    uint256 defaultAssumeValid;

    /**
     * If true, witness commitments contain a payload equal to a Freicoin Script solution
     * to the signet challenge. See BIP325.
     */
    bool signet_blocks{false};
    std::vector<uint8_t> signet_challenge;

    int DeploymentHeight(BuriedDeployment dep) const
    {
        switch (dep) {
        case DEPLOYMENT_HEIGHTINCB:
            return BIP34Height;
        case DEPLOYMENT_DERSIG:
            return BIP66Height;
        case DEPLOYMENT_LOCKTIME:
            return LockTimeHeight;
        case DEPLOYMENT_SEGWIT:
            return SegwitHeight;
        case DEPLOYMENT_CLEANUP:
            return CleanupHeight;
        } // no default case, so the compiler can warn about missing cases
        return std::numeric_limits<int>::max();
    }
};

} // namespace Consensus

/** It's a bit confusing that this is in a consensus header, as the consensus
 ** check requires access to the chain data structures for mean block time.
 ** However running this check with network time is useful for non-consensus
 ** decisions in places where it would be inappropriate to examine the chain
 ** tip.
 **/
inline bool IsProtocolCleanupActive(const Consensus::Params& params, std::chrono::seconds now)
{
    // The adjustment of 3 hours from network time is to allow for some variation
    // in clocks up to a total error of 3 hours.  This prevents nodes from being
    // banned for relaying invalid transactions moments before the switchover.
    return (now.count() > (params.protocol_cleanup_activation_time - /* 3 hours */ 3*60*60));
}
inline bool IsSizeExpansionActive(const Consensus::Params& params, std::chrono::seconds now)
{
    return (now.count() > (params.size_expansion_activation_time - /* 3 hours */ 3*60*60));
}
inline Consensus::RuleSet GetActiveRules(const Consensus::Params& params, std::chrono::seconds now)
{
    Consensus::RuleSet rules = Consensus::NONE;
    if (IsProtocolCleanupActive(params, now)) {
        rules |= Consensus::PROTOCOL_CLEANUP;
    }
    if (IsSizeExpansionActive(params, now)) {
        rules |= Consensus::SIZE_EXPANSION;
    }
    return rules;
}

#endif // FREICOIN_CONSENSUS_PARAMS_H
