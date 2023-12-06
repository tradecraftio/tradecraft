// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MERGEMINE_H
#define BITCOIN_MERGEMINE_H

#include <chainparams.h> // for ChainId
#include <node/context.h>
#include <sync.h>
#include <uint256.h>

#include <univalue.h>

#include <map>
#include <optional>
#include <string>

struct AuxWork {
    uint64_t timestamp;
    std::string job_id;
    std::vector<std::pair<uint8_t, uint256>> path;
    uint256 commit;
    uint32_t bits;
    unsigned char bias;

    AuxWork() : timestamp(0), bits(0x1dffffff), bias(0) { }
    AuxWork(uint64_t timestampIn, const std::string& job_idIn, const uint256& commitIn, uint32_t bitsIn, unsigned char biasIn) : timestamp(timestampIn), job_id(job_idIn), commit(commitIn), bits(bitsIn), bias(biasIn) { }
};

struct AuxProof {
    uint256 midstate_hash;
    std::vector<unsigned char> midstate_buffer;
    uint32_t midstate_length;
    uint32_t lock_time;
    std::vector<uint256> aux_branch;
    uint32_t num_txns;

    uint32_t nVersion;
    uint256 hashPrevBlock;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    AuxProof() : midstate_length(0), lock_time(0), num_txns(0), nVersion(0), nTime(0), nBits(0), nNonce(0) { }
};

struct SecondStageWork {
    uint64_t timestamp;
    double diff;
    std::string job_id;
    uint256 hashPrevBlock;
    std::vector<unsigned char> cb1;
    std::vector<unsigned char> cb2;
    std::vector<uint256> cb_branch;
    uint32_t nVersion;
    uint32_t nBits;
    uint32_t nTime;

    SecondStageWork() : timestamp(0), diff(0), nVersion(0), nBits(0), nTime(0) { }
    SecondStageWork(uint64_t timestampIn, double diffIn, const std::string& job_idIn, const uint256& hashPrevBlockIn, const std::vector<unsigned char>& cb1In, const std::vector<unsigned char>& cb2In, const std::vector<uint256>& cb_branchIn, uint32_t nVersionIn, uint32_t nBitsIn, uint32_t nTimeIn) : timestamp(timestampIn), diff(diffIn), job_id(job_idIn), hashPrevBlock(hashPrevBlockIn), cb1(cb1In), cb2(cb2In), cb_branch(cb_branchIn), nVersion(nVersionIn), nBits(nBitsIn), nTime(nTimeIn) { }
};

struct SecondStageProof {
    std::vector<unsigned char> extranonce1;
    std::vector<unsigned char> extranonce2;
    uint32_t nVersion;
    uint32_t nTime;
    uint32_t nNonce;

    SecondStageProof() : nVersion(0), nTime(0), nNonce(0) { }
    SecondStageProof(const std::vector<unsigned char>& extranonce1In, const std::vector<unsigned char>& extranonce2In, uint32_t nVersionIn, uint32_t nTimeIn, uint32_t nNonceIn) : extranonce1(extranonce1In), extranonce2(extranonce2In), nVersion(nVersionIn), nTime(nTimeIn), nNonce(nNonceIn) { }
};

//! Mapping of alternative names to chain specifiers
extern std::map<std::string, ChainId> chain_names;

//! Critical seciton guarding access to all of the merge-mining global state
extern RecursiveMutex cs_merge_mining;

/** Registers a mining client to a merge-mined chain, so that we will
 ** see auxiliary notifications for that miner on that chain. */
void RegisterMergeMineClient(const ChainId& chainid, const std::string& username, const std::string& password);

/** Takes a mapping of chainid -> username and returns a mapping of chainid -> AuxWork */
std::map<ChainId, AuxWork> GetMergeMineWork(const std::map<ChainId, std::pair<std::string, std::string> >& auth);

/** Returns a second stage work unit, if there is one available.  If the client
 ** is currently working on a second-stage work unit, it is passed as a hint and
 ** the merge-mining subsystem will return the same work if it is still
 ** valid. */
std::optional<std::pair<ChainId, SecondStageWork> > GetSecondStageWork(std::optional<ChainId> hint);

/** Submit share to the specified auxiliary chain. */
void SubmitAuxChainShare(const ChainId& chainid, const std::string& username, const AuxWork& work, const AuxProof& proof);

void SubmitSecondStageShare(const ChainId& chainid, const std::string& username, const SecondStageWork& work, const SecondStageProof& proof);

/** Reconnect to any auxiliary work sources with dropped connections. */
void ReconnectToMergeMineEndpoints();

/** Configure the merge-mining subsystem.  This involved setting up some global
 ** state and spawning initialization and management threads. */
bool InitMergeMining(node::NodeContext& node);

/** Interrupt any active network connections. */
void InterruptMergeMining();

/** Cleanup network connections made by the merge-mining subsystem, free
 ** associated resources, and cleanup global state. */
void StopMergeMining();

#endif // BITCOIN_MERGEMINE_H

// End of File
