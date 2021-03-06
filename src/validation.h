// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
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

#ifndef FREICOIN_VALIDATION_H
#define FREICOIN_VALIDATION_H

#if defined(HAVE_CONFIG_H)
#include "config/freicoin-config.h"
#endif

#include "amount.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "fs.h"
#include "protocol.h" // For CMessageHeader::MessageStartChars
#include "policy/feerate.h"
#include "script/script_error.h"
#include "sync.h"
#include "txmempool.h"
#include "versionbits.h"

#include <algorithm>
#include <exception>
#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include <atomic>

class CBlockIndex;
class CBlockTreeDB;
class CChainParams;
class CCoinsViewDB;
class CInv;
class CConnman;
class CScriptCheck;
class CBlockPolicyEstimator;
class CTxMemPool;
class CValidationState;
struct ChainTxData;

struct PrecomputedTransactionData;
struct LockPoints;

/** Default for DEFAULT_WHITELISTRELAY. */
static const bool DEFAULT_WHITELISTRELAY = true;
/** Default for DEFAULT_WHITELISTFORCERELAY. */
static const bool DEFAULT_WHITELISTFORCERELAY = true;
/** Default for -minrelaytxfee, minimum relay fee for transactions */
static const unsigned int DEFAULT_MIN_RELAY_TX_FEE = 1000;
//! -maxtxfee default
static const CAmount DEFAULT_TRANSACTION_MAXFEE = 0.1 * COIN;
//! Discourage users to set fees higher than this amount (in kria) per kB
static const CAmount HIGH_TX_FEE_PER_KB = 0.01 * COIN;
//! -maxtxfee will warn if called with a higher fee than this amount (in kria)
static const CAmount HIGH_MAX_TX_FEE = 100 * HIGH_TX_FEE_PER_KB;
/** Default for -limitancestorcount, max number of in-mempool ancestors */
static const unsigned int DEFAULT_ANCESTOR_LIMIT = 25;
/** Default for -limitancestorsize, maximum kilobytes of tx + all in-mempool ancestors */
static const unsigned int DEFAULT_ANCESTOR_SIZE_LIMIT = 101;
/** Default for -limitdescendantcount, max number of in-mempool descendants */
static const unsigned int DEFAULT_DESCENDANT_LIMIT = 25;
/** Default for -limitdescendantsize, maximum kilobytes of in-mempool descendants */
static const unsigned int DEFAULT_DESCENDANT_SIZE_LIMIT = 101;
/** Default for -mempoolexpiry, expiration time for mempool transactions in hours */
static const unsigned int DEFAULT_MEMPOOL_EXPIRY = 336;
/** Maximum kilobytes for transactions to store for processing during reorg */
static const unsigned int MAX_DISCONNECTED_TX_POOL_SIZE = 20000;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000; // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB

/** Maximum number of script-checking threads allowed */
static const int MAX_SCRIPTCHECK_THREADS = 16;
/** -par default (number of script-checking threads, 0 = auto) */
static const int DEFAULT_SCRIPTCHECK_THREADS = 0;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
/** Timeout in seconds during which a peer must stall block download progress before being disconnected. */
static const unsigned int BLOCK_STALLING_TIMEOUT = 2;
/** Number of headers sent in one getheaders result. We rely on the assumption that if a peer sends
 *  less than this number, we reached its tip. Changing this value is a protocol upgrade. */
static const unsigned int MAX_HEADERS_RESULTS = 2000;
/** Maximum depth of blocks we're willing to serve as compact blocks to peers
 *  when requested. For older blocks, a regular BLOCK response will be sent. */
static const int MAX_CMPCTBLOCK_DEPTH = 5;
/** Maximum depth of blocks we're willing to respond to GETBLOCKTXN requests for. */
static const int MAX_BLOCKTXN_DEPTH = 10;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and in the future perhaps pruning
 *  harder). We'll probably want to make this a per-peer adaptive value at some point. */
static const unsigned int BLOCK_DOWNLOAD_WINDOW = 1024;
/** Time to wait (in seconds) between writing blocks/block index to disk. */
static const unsigned int DATABASE_WRITE_INTERVAL = 60 * 60;
/** Time to wait (in seconds) between flushing chainstate to disk. */
static const unsigned int DATABASE_FLUSH_INTERVAL = 24 * 60 * 60;
/** Maximum length of reject messages. */
static const unsigned int MAX_REJECT_MESSAGE_LENGTH = 111;
/** Average delay between local address broadcasts in seconds. */
static const unsigned int AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL = 24 * 60 * 60;
/** Average delay between peer address broadcasts in seconds. */
static const unsigned int AVG_ADDRESS_BROADCAST_INTERVAL = 30;
/** Average delay between trickled inventory transmissions in seconds.
 *  Blocks and whitelisted receivers bypass this, outbound peers get half this delay. */
static const unsigned int INVENTORY_BROADCAST_INTERVAL = 5;
/** Maximum number of inventory items to send per transmission.
 *  Limits the impact of low-fee transaction floods. */
static const unsigned int INVENTORY_BROADCAST_MAX = 7 * INVENTORY_BROADCAST_INTERVAL;
/** Average delay between feefilter broadcasts in seconds. */
static const unsigned int AVG_FEEFILTER_BROADCAST_INTERVAL = 10 * 60;
/** Maximum feefilter broadcast delay after significant change. */
static const unsigned int MAX_FEEFILTER_CHANGE_DELAY = 5 * 60;
/** Block download timeout base, expressed in millionths of the block interval (i.e. 10 min) */
static const int64_t BLOCK_DOWNLOAD_TIMEOUT_BASE = 1000000;
/** Additional block download timeout per parallel downloading peer (i.e. 5 min) */
static const int64_t BLOCK_DOWNLOAD_TIMEOUT_PER_PEER = 500000;

static const int64_t DEFAULT_MAX_TIP_AGE = 14 * 24 * 60 * 60;
/** Maximum age of our tip in seconds for us to be considered current for fee estimation */
static const int64_t MAX_FEE_ESTIMATION_TIP_AGE = 3 * 60 * 60;

/** Default for -permitbaremultisig */
static const bool DEFAULT_PERMIT_BAREMULTISIG = true;
static const bool DEFAULT_CHECKPOINTS_ENABLED = true;
static const bool DEFAULT_TXINDEX = false;
static const unsigned int DEFAULT_BANSCORE_THRESHOLD = 100;
/** Default for -persistmempool */
static const bool DEFAULT_PERSIST_MEMPOOL = true;
/** Default for using fee filter */
static const bool DEFAULT_FEEFILTER = true;

/** Maximum number of headers to announce when relaying blocks with headers message.*/
static const unsigned int MAX_BLOCKS_TO_ANNOUNCE = 8;

/** Maximum number of unconnecting headers announcements before DoS score */
static const int MAX_UNCONNECTING_HEADERS = 10;

static const bool DEFAULT_PEERBLOOMFILTERS = true;

/** Scheduled protocol cleanup rule change
 **
 ** Merge mining is implemented as a soft-fork change to the consensus rules to
 ** achieve a safer and less-disruptive deployment than the hard-fork that was
 ** used to deploy merge mining to other chains in the past: non-upgraded
 ** clients will continue to receive blocks at the point of activation with SPV
 ** security.  However the security of successive blocks will diminish as the
 ** difficulty transitions from native to auxiliary proof-of-work.  When the
 ** native difficulty reaches minimal values, there will no longer be any
 ** effective SPV protections for old nodes, and this represents a unique
 ** opportunity to deploy other non-controversial hard-fork changes to the
 ** consensus rules.
 **
 ** For this reason, a protocol-cleanup hard-fork is scheduled to take place
 ** after the activation of merge mining and difficulty transition.  As part of
 ** this cleanup, the following consensus rule changes will take effect:
 **
 **   1. Remove the native proof-of-work requirement entirely.  For reasons of
 **      infrastructure compatibility the block hash will still be the hash of
 **      the native header, but the hash of the header will no longer have to
 **      meet any threshold target.  This makes a winning auxiliary share
 **      automatically a winning block.
 **
 **   2. Remove the MAX_BLOCK_SIGOPS_COST limit.  Switching to libsecp256k1 for
 **      validation and better signature / script and transaction validation
 **      caching has made this limit nearly redundant.
 **
 **   3. Allow a transaction without transaction outputs.  A transaction must
 **      have input(s) to have a unique transaction ID, but it need not have
 **      outputs.  There are obscure cases when this makes sense to do (and thus
 **      forward the funds entirely as "fee" to the miner, or to process in the
 **      block-final transaction and/or coinbase in some way).
 **
 **   4. Do not restrict the contents of the "coinbase string" in any way,
 **      beyond the required auxiliary proof-of-work commitment.  It is
 **      currently required to be between 2 and 100 bytes in size, and must
 **      begin with the serialized block height.  The length restriction is
 **      unnecessary as miners have other means of padding transactions if they
 **      need to, and are generally incentivized not to because of miner fees.
 **      The serialized height requirement is redundant as lock_height is also
 **      required to be set to the current block height.
 **
 **   5. Do not require the coinbase transaction to be final, freeing up
 **      nSequence to be used as the miner's extranonce field.  A previous
 **      soft-fork which required the coinbase's nLockTime field to be set to
 **      the medium-time-past value had the unfortunate side effect of requiring
 **      nSequence to be set to 0xffffffff since even the coinbase is checked
 **      for transaction finality.  The concept of finality makes no sense for
 **      the coinbase and this requirement is dropped after activation of the
 **      new rules, making the 4-byte nSequence field have no consensus-defined
 **      meaning, allowing it to be used as an extranonce field.
 **
 **   6. Do not require zero-valued outputs to be spent by transactions with
 **      lock_height >= the coin's refheight.  This restriction is to ensure
 **      that refheights are always increasing so that demurrage is collected,
 **      not reversed.  However this argument doesn't really make sense for
 **      zero-valued outputs.  At the same time "zero-valued" outputs are
 **      increasingly likely to be used for confidential transactions or
 **      non-freicoin issued assets using extension outputs, for which the
 **      monotonic lock_height requirement is just an annoying protocol
 **      complication.
 **
 **   7. Do not reject "old" blocks after activation of the nVersion=2 and
 **      nVersion=3 soft-forks.  With the switch to version bits for soft-fork
 **      activation, this archaic check is shown to be rather pointless.  Rules
 **      are enforced in a block if it is downstream of the point of activation,
 **      not based on the nVersion value.  Implicitly this also restores
 **      validity of "negative" block.nVersion values.
 **
 **   8. Lift restrictions inside the script interpreter on maximum script size,
 **      maximum data push, maximum number of elements on the stack, and maximum
 **      number of executed opcodes.
 **
 **   9. Remove checks on disabled opcodes, and cause unrecognized opcodes to
 **      "return true" instead of raising an error.
 **
 **   10. Re-enable (and implement) certain disabled opcodes, and conspicuously
 **       missing opcodes which were never there in the first place.
 **
 ** Activation of the protocol-cleanup fork depends on the status of the auxpow
 ** soft-fork, and the median-time-past of the tip relative to a consensus
 ** parameter.  While it makes more logical sense for this to be an inline
 ** method of the chain parameters, doing so would introduce a new dependency on
 ** CBlockIndex there.
 **
 ** There are two implementations that appear to do different things, but
 ** actually are making the same check.  The median-time-past is stored in the
 ** coinbase of the block within the nLockTime field, which allows this check to
 ** be made at points where no chain context is available.
 **/
inline bool IsProtocolCleanupActive(const Consensus::Params& params, const CBlock& block)
{
    if (block.m_aux_pow.IsNull()) {
        return false;
    }
    return ((!block.vtx.empty() ? block.vtx[0]->nLockTime : 0) >= params.protocol_cleanup_activation_time);
}
inline bool IsProtocolCleanupActive(const Consensus::Params& params, const CBlockIndex* pindex)
{
    return ((pindex ? pindex->GetMedianTimePast() : 0) >= params.protocol_cleanup_activation_time);
}

/** Scheduled size expansion rule change
 **
 ** To achieve desired scaling limits, the forward blocks architecture will
 ** eventually trigger a hard-fork modification of the consensus rules, for the
 ** primary purpose of dropping enforcement of many aggregate block limits so as
 ** to allow larger blocks on the compatibility chain.
 **
 ** This hard-fork will not activate until it is absolutely necessary for it to
 ** do so, at the point when real demand for additional shard space in aggregate
 ** across all forward block shard-chains exceeds the available space in the
 ** compatibility chain.  It is anticipated that this will not occur until many,
 ** many years into the future, when Freicoin/Tradecraft's usage exceeds even
 ** the levels of bitcoin usage ca. 2018.  However when it does eventually
 ** trigger, any node enforcing the old rules will be left behind.
 **
 ** Since the rule changes for forward blocks have not been written yet and
 ** because this flag-day code doesn't know how to detect actual activation, we
 ** cannot have older clients enforce the new rules.  What is done instead is
 ** that any rule which we anticipate changing becomes simply unenforced after
 ** this activation time, and aggregate limits are set to the maximum values the
 ** software is able to support.  After the flag-day, older clients of at least
 ** version 13.2.4 will continue to receive blocks, but with only SPV security
 ** ("trust the most work") for the new protocol rules.  So starting with the
 ** release of v13.2.4-11864, activation of forward blocks' new scaling limits
 ** becomes a soft-fork, with the only concern being the forking off of older
 ** nodes upon activation.
 **
 ** The primary rules which must be altered for forward blocks scaling are:
 **
 **   1. Significant relaxation of the rules regarding per-block auxiliary
 **      difficulty adjustment, to allow adjustments of +/- 2x within eleven
 **      blocks, without regard of a target interval.  Forward blocks may have a
 **      new difficulty adjustment algorithm that has yet to be determined, and
 **      might include targeting a variable inter-block time to achieve
 **      compatibility chain scalability.
 **
 **   2. Increase of the maximum block size.  Uncapping the block size is not
 **      possible because even if the explicit limit is removed there are still
 **      implicit network and disk protocol limits that would prevent a client
 **      from syncing a chain with larger blocks.  But these network and disk
 **      limits could be set much higher than the current limits based on a 1
 **      megabyte MAX_BASE_BLOCK_SIZE / 4 megaweight MAX_BLOCK_WEIGHT.
 **
 **   3. Allow larger transactions, up to the new, larger maximum block size
 **      limit in size.  This is less safe than increasing the block size since
 **      most of the nonlinear validation costs are quadratic in transaction
 **      size.  But there is research to be done in choosing what new limits
 **      should be used, and in the mean time keeping transactions only limited
 **      by the (new) block size permits flexibility in that future choice.
 **
 **   4. Reduce coinbase maturity to 1 block.  Once forward blocks has
 **      activated, coinbase maturity is an unnecessary delay to processing the
 **      coinbase payout queue.  It must be at least 1 to prevent miners from
 **      issuing themselves excess funds for the duration of 1 block.
 **
 ** Since we don't know when forward blocks will be deployed and activated, we
 ** schedule these rule changes to occur at the end of the support window for
 ** each client release, which is typically 2 years.  Each new release pushes
 ** back this activation date, and since the new rules are a relaxation of the
 ** old rules older clients will remain compatible so long as a majority of
 ** miners have upgrade and thereby pushed back their activation dates.  When
 ** forward blocks is finally deployed and activated, it will schedule its own
 ** modified rule relaxation to occur after the most distant flag day.
 **/
inline bool IsSizeExpansionActive(const Consensus::Params& params, const CBlock& block)
{
    if (block.m_aux_pow.IsNull()) {
        return false;
    }
    return ((!block.vtx.empty() ? block.vtx[0]->nLockTime : 0) >= params.size_expansion_activation_time);
}
inline bool IsSizeExpansionActive(const Consensus::Params& params, const CBlockIndex* pindex)
{
    return ((pindex ? pindex->GetMedianTimePast() : 0) >= params.size_expansion_activation_time);
}

inline RuleSet GetActiveRules(const Consensus::Params& params, const CBlock& block)
{
    RuleSet rules = 0;
    if (IsProtocolCleanupActive(params, block)) {
        rules |= PROTOCOL_CLEANUP;
    }
    if (IsSizeExpansionActive(params, block)) {
        rules |= SIZE_EXPANSION;
    }
    return rules;
}
inline RuleSet GetActiveRules(const Consensus::Params& params, const CBlockIndex* pindex)
{
    RuleSet rules = 0;
    if (IsProtocolCleanupActive(params, pindex)) {
        rules |= PROTOCOL_CLEANUP;
    }
    if (IsSizeExpansionActive(params, pindex)) {
        rules |= SIZE_EXPANSION;
    }
    return rules;
}

/** A version based on network time, for places in non-consensus code
 ** where it would be inappropriate to examine the chain tip.
 **/
inline bool IsProtocolCleanupActive(const Consensus::Params& params, int64_t now)
{
    return (now > (params.protocol_cleanup_activation_time - 3*60*60 /* three hours */));
}
inline bool IsSizeExpansionActive(const Consensus::Params& params, int64_t now)
{
    return (now > (params.size_expansion_activation_time - 3*60*60 /* three hours */));
}
inline RuleSet GetActiveRules(const Consensus::Params& params, int64_t now)
{
    RuleSet rules = 0;
    if (IsProtocolCleanupActive(params, now)) {
        rules |= PROTOCOL_CLEANUP;
    }
    if (IsSizeExpansionActive(params, now)) {
        rules |= SIZE_EXPANSION;
    }
    return rules;
}

/** Default for -stopatheight */
static const int DEFAULT_STOPATHEIGHT = 0;

struct BlockHasher
{
    size_t operator()(const uint256& hash) const { return hash.GetCheapHash(); }
};

extern CScript COINBASE_FLAGS;
extern CCriticalSection cs_main;
extern CBlockPolicyEstimator feeEstimator;
extern CTxMemPool mempool;
typedef std::unordered_map<uint256, CBlockIndex*, BlockHasher> BlockMap;
extern BlockMap mapBlockIndex;
extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockWeight;
extern const std::string strMessageMagic;
extern CWaitableCriticalSection csBestBlock;
extern CConditionVariable cvBlockChange;
extern std::atomic_bool fImporting;
extern bool fReindex;
extern int nScriptCheckThreads;
extern bool fTxIndex;
extern bool fIsBareMultisigStd;
extern bool fRequireStandard;
extern bool fCheckBlockIndex;
extern bool fCheckpointsEnabled;
extern size_t nCoinCacheUsage;
/** A fee rate smaller than this is considered zero fee (for relaying, mining and transaction creation) */
extern CFeeRate minRelayTxFee;
/** Absolute maximum transaction fee (in kria) used by wallet and mempool (rejects high fee in sendrawtransaction) */
extern CAmount maxTxFee;
/** If the tip is older than this (in seconds), the node is considered to be in initial block download. */
extern int64_t nMaxTipAge;

/** Block hash whose ancestors we will assume to have valid scripts without checking them. */
extern uint256 hashAssumeValid;

/** Minimum work we will assume exists on some valid chain. */
extern arith_uint256 nMinimumChainWork;

/** Best header we've seen so far (used for getheaders queries' starting points). */
extern CBlockIndex *pindexBestHeader;

/** Minimum disk space required - used in CheckDiskSpace() */
static const uint64_t nMinDiskSpace = 52428800;

/** Pruning-related variables and constants */
/** True if any block files have ever been pruned. */
extern bool fHavePruned;
/** True if we're running in -prune mode. */
extern bool fPruneMode;
/** Number of MiB of block files that we're trying to stay below. */
extern uint64_t nPruneTarget;
/** Block files containing a block-height within MIN_BLOCKS_TO_KEEP of chainActive.Tip() will not be pruned. */
static const unsigned int MIN_BLOCKS_TO_KEEP = 288;

static const signed int DEFAULT_CHECKBLOCKS = 6;
static const unsigned int DEFAULT_CHECKLEVEL = 3;

// Require that user allocate at least 550MB for block & undo files (blk???.dat and rev???.dat)
// At 1MB per block, 288 blocks = 288MB.
// Add 15% for Undo data = 331MB
// Add 20% for Orphan block rate = 397MB
// We want the low water mark after pruning to be at least 397 MB and since we prune in
// full block file chunks, we need the high water mark which triggers the prune to be
// one 128MB block file + added 15% undo data = 147MB greater for a total of 545MB
// Setting the target to > than 550MB will make it likely we can respect the target.
static const uint64_t MIN_DISK_SPACE_FOR_BLOCK_FILES = 550 * 1024 * 1024;

/** 
 * Process an incoming block. This only returns after the best known valid
 * block is made active. Note that it does not, however, guarantee that the
 * specific block passed to it has been checked for validity!
 *
 * If you want to *possibly* get feedback on whether pblock is valid, you must
 * install a CValidationInterface (see validationinterface.h) - this will have
 * its BlockChecked method called whenever *any* block completes validation.
 *
 * Note that we guarantee that either the proof-of-work is valid on pblock, or
 * (and possibly also) BlockChecked will have been called.
 * 
 * Call without cs_main held.
 *
 * @param[in]   pblock  The block we want to process.
 * @param[in]   fForceProcessing Process this block even if unrequested; used for non-network block sources and whitelisted peers.
 * @param[out]  fNewBlock A boolean which is set to indicate if the block was first received via this call
 * @return True if state.IsValid()
 */
bool ProcessNewBlock(const CChainParams& chainparams, const std::shared_ptr<const CBlock> pblock, bool fForceProcessing, bool* fNewBlock);

/**
 * Process incoming block headers.
 *
 * Call without cs_main held.
 *
 * @param[in]  block The block headers themselves
 * @param[out] state This may be set to an Error state if any error occurred processing them
 * @param[in]  chainparams The params for the chain we want to connect to
 * @param[out] ppindex If set, the pointer will be set to point to the last new block index object for the given headers
 * @param[out] first_invalid First header that fails validation, if one exists
 */
bool ProcessNewBlockHeaders(const std::vector<CBlockHeader>& block, CValidationState& state, const CChainParams& chainparams, const CBlockIndex** ppindex=nullptr, CBlockHeader *first_invalid=nullptr);

/** Check whether enough disk space is available for an incoming block */
bool CheckDiskSpace(uint64_t nAdditionalBytes = 0);
/** Open a block file (blk?????.dat) */
FILE* OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly = false);
/** Translation to a filesystem path */
fs::path GetBlockPosFilename(const CDiskBlockPos &pos, const char *prefix);
/** Import blocks from an external file */
bool LoadExternalBlockFile(const CChainParams& chainparams, FILE* fileIn, CDiskBlockPos *dbp = nullptr);
/** Ensures we have a genesis block in the block tree, possibly writing one to disk. */
bool LoadGenesisBlock(const CChainParams& chainparams);
/** Load the block tree and coins database from disk,
 * initializing state if we're running with -reindex. */
bool LoadBlockIndex(const CChainParams& chainparams);
/** Update the chain tip based on database information. */
bool LoadChainTip(const CChainParams& chainparams);
/** Unload database information */
void UnloadBlockIndex();
/** Run an instance of the script checking thread */
void ThreadScriptCheck();
/** Check whether we are doing an initial block download (synchronizing from disk or network) */
bool IsInitialBlockDownload();
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(const uint256 &hash, CTransactionRef &tx, const Consensus::Params& params, uint256 &hashBlock, bool fAllowSlow = false);
/** Find the best known block, and make it the tip of the block chain */
bool ActivateBestChain(CValidationState& state, const CChainParams& chainparams, std::shared_ptr<const CBlock> pblock = std::shared_ptr<const CBlock>());
CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams);

/** Guess verification progress (as a fraction between 0.0=genesis and 1.0=current tip). */
double GuessVerificationProgress(const ChainTxData& data, CBlockIndex* pindex);

/**
 *  Mark one block file as pruned.
 */
void PruneOneBlockFile(const int fileNumber);

/**
 *  Actually unlink the specified files
 */
void UnlinkPrunedFiles(const std::set<int>& setFilesToPrune);

/** Create a new block index entry for a given block hash */
CBlockIndex * InsertBlockIndex(uint256 hash);
/** Flush all state, indexes and buffers to disk. */
void FlushStateToDisk();
/** Prune block files and flush state to disk. */
void PruneAndFlush();
/** Prune block files up to a given height */
void PruneBlockFilesManual(int nManualPruneHeight);

/** (try to) add transaction to memory pool
 * plTxnReplaced will be appended to with all transactions replaced from mempool **/
bool AcceptToMemoryPool(CTxMemPool& pool, CValidationState &state, const CTransactionRef &tx, bool fLimitFree,
                        bool* pfMissingInputs, std::list<CTransactionRef>* plTxnReplaced = nullptr,
                        bool fOverrideMempoolLimit=false, const CAmount nAbsurdFee=0);

/** Convert CValidationState to a human-readable message for logging */
std::string FormatStateMessage(const CValidationState &state);

/** Get the BIP9 state for a given deployment at the current tip. */
ThresholdState VersionBitsTipState(const Consensus::Params& params, Consensus::DeploymentPos pos);

/** Get the numerical statistics for the BIP9 state for a given deployment at the current tip. */
BIP9Stats VersionBitsTipStatistics(const Consensus::Params& params, Consensus::DeploymentPos pos);

/** Get the block height at which the BIP9 deployment switched into the state for the block building on the current tip. */
int VersionBitsTipStateSinceHeight(const Consensus::Params& params, Consensus::DeploymentPos pos);


/** Apply the effects of this transaction on the UTXO set represented by view */
void UpdateCoins(const CTransaction& tx, CCoinsViewCache& inputs, int nHeight);

/** Transaction validation functions */

/**
 * Check whether the specified output of the coin/transaction can be spent with
 * an empty scriptSig.
 */
bool IsTriviallySpendable(const Coin& from, const COutPoint& prevout, unsigned int flags);
bool IsTriviallySpendable(const CTransaction& txFrom, uint32_t n, unsigned int flags);

/**
 * Check if transaction will be final in the next block to be created.
 *
 * Calls IsFinalTx() with current block height and appropriate block time.
 *
 * See consensus/consensus.h for flag definitions.
 */
bool CheckFinalTx(const CTransaction &tx, int flags = -1);

/**
 * Test whether the LockPoints height and time are still valid on the current chain
 */
bool TestLockPointValidity(const LockPoints* lp);

/**
 * Check if transaction will be BIP 68 final in the next block to be created.
 *
 * Simulates calling SequenceLocks() with data from the tip of the current active chain.
 * Optionally stores in LockPoints the resulting height and time calculated and the hash
 * of the block needed for calculation or skips the calculation and uses the LockPoints
 * passed in for evaluation.
 * The LockPoints should not be considered valid if CheckSequenceLocks returns false.
 *
 * See consensus/consensus.h for flag definitions.
 */
bool CheckSequenceLocks(const CTransaction &tx, int flags, LockPoints* lp = nullptr, bool useExistingLockPoints = false);

/**
 * Closure representing one script verification
 * Note that this stores references to the spending transaction 
 */
class CScriptCheck
{
private:
    CScript scriptPubKey;
    CAmount amount;
    int64_t refheight;
    const CTransaction *ptxTo;
    unsigned int nIn;
    unsigned int nFlags;
    bool cacheStore;
    ScriptError error;
    PrecomputedTransactionData *txdata;

public:
    CScriptCheck(): amount(0), refheight(0), ptxTo(0), nIn(0), nFlags(0), cacheStore(false), error(SCRIPT_ERR_UNKNOWN_ERROR) {}
    CScriptCheck(const CScript& scriptPubKeyIn, const CAmount amountIn, int64_t refheightIn, const CTransaction& txToIn, unsigned int nInIn, unsigned int nFlagsIn, bool cacheIn, PrecomputedTransactionData* txdataIn) :
        scriptPubKey(scriptPubKeyIn), amount(amountIn), refheight(refheightIn),
        ptxTo(&txToIn), nIn(nInIn), nFlags(nFlagsIn), cacheStore(cacheIn), error(SCRIPT_ERR_UNKNOWN_ERROR), txdata(txdataIn) { }

    bool operator()();

    void swap(CScriptCheck &check) {
        scriptPubKey.swap(check.scriptPubKey);
        std::swap(ptxTo, check.ptxTo);
        std::swap(amount, check.amount);
        std::swap(refheight, check.refheight);
        std::swap(nIn, check.nIn);
        std::swap(nFlags, check.nFlags);
        std::swap(cacheStore, check.cacheStore);
        std::swap(error, check.error);
        std::swap(txdata, check.txdata);
    }

    ScriptError GetScriptError() const { return error; }
};

/** Initializes the script-execution cache */
void InitScriptExecutionCache();


/** Functions for disk access for blocks */
bool ReadBlockFromDisk(CBlock& block, const CDiskBlockPos& pos, const Consensus::Params& consensusParams);
bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);

/** Functions for validating blocks and updating the block tree */

/** Context-independent validity checks */
bool CheckBlock(const CBlock& block, CValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW = true, bool fCheckMerkleRoot = true);

/** Check a block is completely valid from start to finish (only works on top of our current best block, with cs_main held) */
bool TestBlockValidity(CValidationState& state, const CChainParams& chainparams, const CBlock& block, CBlockIndex* pindexPrev, bool fCheckPOW = true, bool fCheckMerkleRoot = true);

/** Check whether block-final transaction rules are enforced for block. */
bool IsFinalTxEnforced(const CBlockIndex* pindexPrev, const Consensus::Params& params);

/** Check whether witness commitments are required for block. */
bool IsWitnessEnabled(const CBlockIndex* pindexPrev, const Consensus::Params& params);

/** Check whether merge mining is required for block. */
bool IsMergeMiningEnabled(const CBlockIndex* pindexPrev, const Consensus::Params& params);

/** When there are blocks in the active chain with missing data, rewind the chainstate and remove them from the block index */
bool RewindBlockIndex(const CChainParams& params);

/** Compute at which vout of the block's coinbase transaction the witness commitment occurs, or -1 if not found. */
bool GetWitnessCommitment(const CBlock& block, unsigned char* path, uint256* hash);

/** Update uncommitted block structures (currently: only the witness nonce). This is safe for submitted blocks. */
void UpdateUncommittedBlockStructures(CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams);

/** Produce the necessary coinbase commitment for a block (modifies the hash, don't call for mined blocks). */
void GenerateCoinbaseCommitment(CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams);

/** RAII wrapper for VerifyDB: Verify consistency of the block and coin databases */
class CVerifyDB {
public:
    CVerifyDB();
    ~CVerifyDB();
    bool VerifyDB(const CChainParams& chainparams, CCoinsView *coinsview, int nCheckLevel, int nCheckDepth);
};

/** Replay blocks that aren't fully applied to the database. */
bool ReplayBlocks(const CChainParams& params, CCoinsView* view);

/** Find the last common block between the parameter chain and a locator. */
CBlockIndex* FindForkInGlobalIndex(const CChain& chain, const CBlockLocator& locator);

/** Mark a block as precious and reorganize. */
bool PreciousBlock(CValidationState& state, const CChainParams& params, CBlockIndex *pindex);

/** Mark a block as invalid. */
bool InvalidateBlock(CValidationState& state, const CChainParams& chainparams, CBlockIndex *pindex);

/** Remove invalidity status from a block and its descendants. */
bool ResetBlockFailureFlags(CBlockIndex *pindex);

/** The currently-connected chain of blocks (protected by cs_main). */
extern CChain chainActive;

/** Global variable that points to the coins database (protected by cs_main) */
extern CCoinsViewDB *pcoinsdbview;

/** Global variable that points to the active CCoinsView (protected by cs_main) */
extern CCoinsViewCache *pcoinsTip;

/** Global variable that points to the active block tree (protected by cs_main) */
extern CBlockTreeDB *pblocktree;

/**
 * Return the spend height, which is one more than the inputs.GetBestBlock().
 * While checking, GetBestBlock() refers to the parent block. (protected by cs_main)
 * This is also true for mempool checks.
 */
int GetSpendHeight(const CCoinsViewCache& inputs);

extern VersionBitsCache versionbitscache;

/**
 * Determine what nVersion a new block should use.
 */
int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params);

/** Reject codes greater or equal to this can be returned by AcceptToMemPool
 * for transactions, to signal internal conditions. They cannot and should not
 * be sent over the P2P network.
 */
static const unsigned int REJECT_INTERNAL = 0x100;
/** Too high fee. Can not be triggered by P2P transactions */
static const unsigned int REJECT_HIGHFEE = 0x100;

/** Get block file info entry for one block file */
CBlockFileInfo* GetBlockFileInfo(size_t n);

/** Dump the mempool to disk. */
void DumpMempool();

/** Load the mempool from disk. */
bool LoadMempool();

#endif // FREICOIN_VALIDATION_H
