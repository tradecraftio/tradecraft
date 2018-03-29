// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include "amount.h"
#include "uint256.h"
#include <map>
#include <string>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_LOCKTIME, // Deployment of BIP68 and BIP113.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    DEPLOYMENT_FINALTX, // Deployment of block-final miner commitment transaction.
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
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    /** Bitcoin unit test compatibility mode */
    bool bitcoin_mode;
    int nSubsidyHalvingInterval;
    /** Demurrage settings */
    CAmount perpetual_subsidy; // equilibrium_monetary_base * demurrage rate
    /** Initial distribution via excess subsidy */
    int64_t equilibrium_height;
    int64_t equilibrium_monetary_base;
    CAmount initial_excess_subsidy;
    /** Soft-fork activations */
    int64_t verify_dersig_activation_height;
    int64_t truncate_inputs_activation_height;
    int64_t alu_activation_height;
    /** Used to check majorities for block version upgrade */
    int nMajorityEnforceBlockUpgrade;
    int nMajorityRejectBlockOutdated;
    int nMajorityWindow;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargetting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    /** Difficulty adjustment parameters */
    int64_t nPowTargetSpacing;
    int64_t original_adjust_interval;
    int64_t filtered_adjust_interval;
    int64_t diff_adjust_threshold;
    int64_t OriginalTargetTimespan() const { return original_adjust_interval * nPowTargetSpacing; }
    int64_t FilteredTargetTimespan() const { return filtered_adjust_interval * nPowTargetSpacing; }
    uint256 nMinimumChainWork;
};
} // namespace Consensus

#endif // FREICOIN_CONSENSUS_PARAMS_H
