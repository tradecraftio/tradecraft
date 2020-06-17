// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <amount.h>
#include <uint256.h>
#include <limits>
#include <map>
#include <string>

namespace Consensus {

typedef unsigned int RuleSet;
static const RuleSet PROTOCOL_CLEANUP = 1;
static const RuleSet SIZE_EXPANSION   = 2;

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_LOCKTIME, // Deployment of BIP68 and BIP113.
    DEPLOYMENT_FINALTX, // Deployment of block-final miner commitment transaction.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    DEPLOYMENT_AUXPOW, // Deployment of merge mining.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;
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
    int64_t verify_coinbase_lock_time_timeout;
    /* Block hash that is excepted from BIP16 enforcement */
    uint256 BIP16Exception;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
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
    bool fPowNoRetargeting;
    /** Difficulty adjustment parameters */
    int64_t nPowTargetSpacing;
    int64_t original_adjust_interval;
    int64_t filtered_adjust_interval;
    int64_t diff_adjust_threshold;
    int64_t OriginalTargetTimespan() const { return original_adjust_interval * nPowTargetSpacing; }
    int64_t FilteredTargetTimespan() const { return filtered_adjust_interval * nPowTargetSpacing; }
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;
};
} // namespace Consensus

/** It's a bit confusing that this is in a consensus header, as the consensus
 ** check requires access to the chain data structures for mean block time.
 ** However running this check with network time is useful for non-consensus
 ** decisions in places where it would be inappropriate to examine the chain
 ** tip.
 **/
inline bool IsProtocolCleanupActive(const Consensus::Params& params, int64_t now)
{
    return (now > (params.protocol_cleanup_activation_time - 3*60*60 /* three hours */));
}
inline bool IsSizeExpansionActive(const Consensus::Params& params, int64_t now)
{
    return (now > (params.size_expansion_activation_time - 3*60*60 /* three hours */));
}
inline Consensus::RuleSet GetActiveRules(const Consensus::Params& params, int64_t now)
{
    Consensus::RuleSet rules = 0;
    if (IsProtocolCleanupActive(params, now)) {
        rules |= Consensus::PROTOCOL_CLEANUP;
    }
    if (IsSizeExpansionActive(params, now)) {
        rules |= Consensus::SIZE_EXPANSION;
    }
    return rules;
}

#endif // FREICOIN_CONSENSUS_PARAMS_H
