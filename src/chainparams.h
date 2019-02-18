// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2011-2019 The Freicoin developers
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

#ifndef FREICOIN_CHAINPARAMS_H
#define FREICOIN_CHAINPARAMS_H

#include "chainparamsbase.h"
#include "checkpoints.h"
#include "primitives/block.h"
#include "protocol.h"
#include "uint256.h"

#include <vector>

typedef unsigned char MessageStartChars[MESSAGE_START_SIZE];

struct CDNSSeedData {
    std::string name, host;
    CDNSSeedData(const std::string &strName, const std::string &strHost) : name(strName), host(strHost) {}
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * Freicoin system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,

        MAX_BASE58_TYPES
    };

    const uint256& HashGenesisBlock() const { return hashGenesisBlock; }
    const MessageStartChars& MessageStart() const { return pchMessageStart; }
    const std::vector<unsigned char>& AlertKey() const { return vAlertPubKey; }
    int GetDefaultPort() const { return nDefaultPort; }
    const uint256& ProofOfWorkLimit() const { return bnProofOfWorkLimit; }
    int SubsidyHalvingInterval() const { return nSubsidyHalvingInterval; }
    /** Used to check majorities for block version upgrade */
    int EnforceBlockUpgradeMajority() const { return nEnforceBlockUpgradeMajority; }
    int RejectBlockOutdatedMajority() const { return nRejectBlockOutdatedMajority; }
    int ToCheckBlockUpgradeMajority() const { return nToCheckBlockUpgradeMajority; }
    /** Used to check majorities for hybrid version bits upgrade */
    int ActivateBitUpgradeMajority() const { return activate_bit_upgrade_majority; }
    int ToCheckBitUpgradeMajority() const { return to_check_bit_upgrade_majority; }

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
     * release of v10.4-8, activation of forward blocks new scaling limits
     * becomes a soft-fork, with the only concern being the forking off of
     * 0.9 series and earlier nodes upon activation.
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
    int64_t ProtocolCleanupActivationTime() const { return protocol_cleanup_activation_time; }

    /** Used if GenerateFreicoins is called with a negative number of threads */
    int DefaultMinerThreads() const { return nMinerThreads; }
    const CBlock& GenesisBlock() const { return genesis; }
    bool RequireRPCPassword() const { return fRequireRPCPassword; }
    /** Make miner wait to have peers to avoid wasting work */
    bool MiningRequiresPeers() const { return fMiningRequiresPeers; }
    /** Default value for -checkmempool and -checkblockindex argument */
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    /** Allow mining of a min-difficulty block */
    bool AllowMinDifficultyBlocks() const { return fAllowMinDifficultyBlocks; }
    /** Skip proof-of-work check: allow mining of any difficulty block */
    bool SkipProofOfWorkCheck() const { return fSkipProofOfWorkCheck; }
    /** Make standard checks */
    bool RequireStandard() const { return fRequireStandard; }
    int64_t TargetSpacing() const { return target_spacing; }
    int64_t OriginalInterval() const { return original_interval; }
    int64_t FilteredInterval() const { return filtered_interval; }
    int64_t OriginalTargetTimespan() const { return original_interval * target_spacing; }
    int64_t FilteredTargetTimespan() const { return filtered_interval * target_spacing; }
    int64_t DiffAdjustThreshold() const { return diff_adjust_threshold; }
    int64_t AluActivationHeight() const { return alu_activation_height; }
    int64_t VerifyCoinbaseLockTimeMinimumHeight() const { return verify_coinbase_lock_time_minimum_height; }
    int64_t VerifyCoinbaseLockTimeActivationTime() const { return verify_coinbase_lock_time_activation_time; }
    int64_t MaxTipAge() const { return nMaxTipAge; }
    /** Make miner stop after a block is found. In RPC, don't return until nGenProcLimit blocks are generated */
    bool MineBlocksOnDemand() const { return fMineBlocksOnDemand; }
    /** In the future use NetworkIDString() for RPC fields */
    bool TestnetToBeDeprecatedFieldRPC() const { return fTestnetToBeDeprecatedFieldRPC; }
    /** Return the BIP70 network string (main, test or regtest) */
    std::string NetworkIDString() const { return strNetworkID; }
    const std::vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    const std::vector<CAddress>& FixedSeeds() const { return vFixedSeeds; }
    virtual const Checkpoints::CCheckpointData& Checkpoints() const = 0;
protected:
    CChainParams() {}

    uint256 hashGenesisBlock;
    MessageStartChars pchMessageStart;
    //! Raw pub key bytes for the broadcast alert signing key.
    std::vector<unsigned char> vAlertPubKey;
    int nDefaultPort;
    uint256 bnProofOfWorkLimit;
    int nSubsidyHalvingInterval;
    int nEnforceBlockUpgradeMajority;
    int nRejectBlockOutdatedMajority;
    int nToCheckBlockUpgradeMajority;
    int activate_bit_upgrade_majority;
    int to_check_bit_upgrade_majority;
    int64_t protocol_cleanup_activation_time;
    int64_t target_spacing;
    int64_t original_interval;
    int64_t filtered_interval;
    int64_t diff_adjust_threshold;
    int64_t alu_activation_height;
    int64_t verify_coinbase_lock_time_minimum_height;
    int64_t verify_coinbase_lock_time_activation_time;
    int nMinerThreads;
    long nMaxTipAge;
    std::vector<CDNSSeedData> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    CBaseChainParams::Network networkID;
    std::string strNetworkID;
    CBlock genesis;
    std::vector<CAddress> vFixedSeeds;
    bool fRequireRPCPassword;
    bool fMiningRequiresPeers;
    bool fAllowMinDifficultyBlocks;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool fMineBlocksOnDemand;
    bool fSkipProofOfWorkCheck;
    bool fTestnetToBeDeprecatedFieldRPC;
};

/** 
 * Modifiable parameters interface is used by test cases to adapt the parameters in order
 * to test specific features more easily. Test cases should always restore the previous
 * values after finalization.
 */

class CModifiableParams {
public:
    //! Published setters to allow changing values in unit test cases
    virtual void setSubsidyHalvingInterval(int anSubsidyHalvingInterval) =0;
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority)=0;
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority)=0;
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority)=0;
    virtual void setDefaultConsistencyChecks(bool aDefaultConsistencyChecks)=0;
    virtual void setAllowMinDifficultyBlocks(bool aAllowMinDifficultyBlocks)=0;
    virtual void setSkipProofOfWorkCheck(bool aSkipProofOfWorkCheck)=0;
};


/**
 * Return the currently selected parameters. This won't change after app startup
 * outside of the unit tests.
 */
const CChainParams &Params();

/** Return parameters for the given network. */
CChainParams &Params(CBaseChainParams::Network network);

/** Get modifiable network parameters (UNITTEST only) */
CModifiableParams *ModifiableParams();

/** Sets the params returned by Params() to those for the given network. */
void SelectParams(CBaseChainParams::Network network);

/**
 * Looks for -regtest or -testnet and then calls SelectParams as appropriate.
 * Returns false if an invalid combination is given.
 */
bool SelectParamsFromCommandLine();

#endif // FREICOIN_CHAINPARAMS_H
