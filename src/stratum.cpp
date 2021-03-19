// Copyright (c) 2020-2021 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "stratum.h"

#include "base58.h"
#include "chainparams.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "crypto/sha256.h"
#include "httpserver.h"
#include "main.h"
#include "miner.h"
#include "netbase.h"
#include "net.h"
#include "rpc/server.h"
#include "serialize.h"
#include "streams.h"
#include "sync.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"

#include <univalue.h>

#include <algorithm> // for std::reverse
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp> // for boost::trim
#include <boost/lexical_cast.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <errno.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

struct StratumClient
{
    evconnlistener* m_listener;
    evutil_socket_t m_socket;
    bufferevent* m_bev;
    CService m_from;
    int m_nextid;
    uint256 m_secret;

    CService GetPeer() const
      { return m_from; }

    std::string m_client;

    bool m_authorized;
    CFreicoinAddress m_addr;
    double m_mindiff;

    uint32_t m_version_rolling_mask;

    CBlockIndex* m_last_tip;
    bool m_second_stage;
    bool m_send_work;

    bool m_supports_aux;
    std::set<CFreicoinAddress> m_aux_addr;

    bool m_supports_extranonce;

    StratumClient() : m_listener(0), m_socket(0), m_bev(0), m_nextid(0), m_authorized(false), m_mindiff(0.0), m_version_rolling_mask(0x00000000), m_last_tip(0), m_second_stage(false), m_send_work(false), m_supports_aux(false), m_supports_extranonce(false) { GenSecret(); }
    StratumClient(evconnlistener* listener, evutil_socket_t socket, bufferevent* bev, CService from) : m_listener(listener), m_socket(socket), m_bev(bev), m_nextid(0), m_from(from), m_authorized(false), m_mindiff(0.0), m_version_rolling_mask(0x00000000), m_last_tip(0), m_second_stage(false), m_send_work(false), m_supports_aux(false), m_supports_extranonce(false) { GenSecret(); }

    void GenSecret();
    std::vector<unsigned char> ExtraNonce1(uint256 job_id) const;
};

void StratumClient::GenSecret()
{
    GetRandBytes(m_secret.begin(), 32);
}

std::vector<unsigned char> StratumClient::ExtraNonce1(uint256 job_id) const
{
    CSHA256 nonce_hasher;
    nonce_hasher.Write(m_secret.begin(), 32);
    if (m_supports_extranonce) {
        nonce_hasher.Write(job_id.begin(), 32);
    }
    uint256 job_nonce;
    nonce_hasher.Finalize(job_nonce.begin());
    return {job_nonce.begin(), job_nonce.begin()+8};
}

struct StratumWork {
    CBlockTemplate m_block_template;
    // First we generate the segwit commitment for the miner's coinbase with
    // ComputeFastMerkleBranch.
    std::vector<uint256> m_cb_wit_branch;
    // Then we compute the initial right-branch for the block-tx Merkle tree
    // using ComputeStableMerkleBranch...
    std::vector<uint256> m_bf_branch;
    // ...which is appended to the end of m_cb_branch so we can compute the
    // block's hashMerkleRoot with ComputeMerkleBranch.
    std::vector<uint256> m_cb_branch;
    bool m_is_witness_enabled;

    // The cached 2nd-stage auxiliary hash value, if an auxiliary proof-of-work
    // solution has been found.
    boost::optional<uint256> m_aux_hash2;

    StratumWork() : m_is_witness_enabled(false) { };
    StratumWork(const CBlockTemplate& block_template, bool is_witness_enabled);

    CBlock& GetBlock()
      { return m_block_template.block; }
    const CBlock& GetBlock() const
      { return m_block_template.block; }
};

StratumWork::StratumWork(const CBlockTemplate& block_template, bool is_witness_enabled)
    : m_block_template(block_template)
    , m_is_witness_enabled(is_witness_enabled)
{
    // Generate the block-witholding secret for the work unit.
    if (!m_block_template.block.m_aux_pow.IsNull()) {
        GetRandBytes((unsigned char*)&m_block_template.block.m_aux_pow.m_secret_lo, 8);
        GetRandBytes((unsigned char*)&m_block_template.block.m_aux_pow.m_secret_hi, 8);
    }
    // How we use the various branch fields depends on whether segregated
    // witness is active.  If segwit is not active, m_cb_branch contains the
    // Merkle proof for the coinbase.  If segwit is active, we also use this
    // field in a different way, so we compute it in both branches.
    std::vector<uint256> leaves;
    for (const auto& tx : m_block_template.block.vtx) {
        leaves.push_back(tx.GetHash());
    }
    m_cb_branch = ComputeMerkleBranch(leaves, 0);
    // If segwit is not active, we're done.  Otherwise...
    if (m_is_witness_enabled) {
        // The final hash in m_cb_branch is the right-hand branch from
        // the root, which contains the block-final transaction (and
        // therefore the segwit commitment).
        m_cb_branch.pop_back();
        // To calculate the initial right-side hash, we need the path
        // to the root from the coinbase's position.  Again, the final
        // hash won't be known ahead of time because it depends on the
        // contents of the coinbase (which depends on both the miner's
        // payout address and the specific extranonce2 used).
        m_bf_branch = ComputeStableMerkleBranch(leaves, leaves.size()-1).first;
        m_bf_branch.pop_back();
        // To calculate the segwit commitment for the block-final tx,
        // we use a proof from the coinbase's position of the witness
        // Merkle tree.
        for (int i = 1; i < m_block_template.block.vtx.size()-1; ++i) {
            leaves[i] = m_block_template.block.vtx[i].GetWitnessHash();
        }
        CMutableTransaction bf(m_block_template.block.vtx.back());
        CScript& scriptPubKey = bf.vout.back().scriptPubKey;
        if (scriptPubKey.size() < 37) {
            throw std::runtime_error("Expected last output of block-final transaction to have enough room for segwit commitment, but alas.");
        }
        std::fill_n(&scriptPubKey[scriptPubKey.size()-37], 33, 0x00);
        leaves.back() = bf.GetHash();
        m_cb_wit_branch = ComputeFastMerkleBranch(leaves, 0).first;
    }
};

void UpdateSegwitCommitment(const StratumWork& current_work, CMutableTransaction& cb, CMutableTransaction& bf, std::vector<uint256>& cb_branch)
{
    // Calculate witnessroot
    CMutableTransaction cb2(cb);
    cb2.vin[0].scriptSig = CScript();
    cb2.vin[0].nSequence = 0;
    auto witnessroot = ComputeFastMerkleRootFromBranch(cb2.GetHash(), current_work.m_cb_wit_branch, 0, nullptr);

    // Build block-final tx
    CScript& scriptPubKey = bf.vout.back().scriptPubKey;
    scriptPubKey[scriptPubKey.size()-37] = 0x01;
    std::copy(witnessroot.begin(),
              witnessroot.end(),
              &scriptPubKey[scriptPubKey.size()-36]);

    // Calculate right-branch
    auto pathmask = ComputeMerklePathAndMask(current_work.m_bf_branch.size() + 1, current_work.GetBlock().vtx.size() - 1);
    cb_branch.push_back(ComputeStableMerkleRootFromBranch(bf.GetHash(), current_work.m_bf_branch, pathmask.first, pathmask.second, nullptr));
}

//! Critical seciton guarding access to any of the stratum global state
static CCriticalSection cs_stratum;

//! List of subnets to allow stratum connections from
static std::vector<CSubNet> stratum_allow_subnets;

//! Bound stratum listening sockets
static std::map<evconnlistener*, CService> bound_listeners;

//! Active miners connected to us
static std::map<bufferevent*, StratumClient> subscriptions;

//! Mapping of stratum method names -> handlers
static std::map<std::string, boost::function<UniValue(StratumClient&, const UniValue&)> > stratum_method_dispatch;

//! A mapping of job_id -> work templates
static std::map<uint256, StratumWork> work_templates;

//! The job_id of the first work unit to have its auxiliary proof-of-work solved
//! for the current block, or boost::none if no solution has been returned yet.
static boost::optional<uint256> half_solved_work;

//! A thread to watch for new blocks and send mining notifications
static boost::thread block_watcher_thread;

std::string HexInt4(uint32_t val)
{
    std::vector<unsigned char> vch;
    vch.push_back((val >> 24) & 0xff);
    vch.push_back((val >> 16) & 0xff);
    vch.push_back((val >>  8) & 0xff);
    vch.push_back( val        & 0xff);
    return HexStr(vch);
}

uint32_t ParseHexInt4(const UniValue& hex, const std::string& name)
{
    std::vector<unsigned char> vch = ParseHexV(hex, name);
    if (vch.size() != 4) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, name+" must be exactly 4 bytes / 8 hex");
    }
    uint32_t ret = 0;
    ret |= vch[0] << 24;
    ret |= vch[1] << 16;
    ret |= vch[2] <<  8;
    ret |= vch[3];
    return ret;
}

uint256 ParseUInt256(const UniValue& hex, const std::string& name)
{
    if (!hex.isStr()) {
        throw std::runtime_error(name+" must be a hexidecimal string");
    }
    std::vector<unsigned char> vch = ParseHex(hex.get_str());
    if (vch.size() != 32) {
        throw std::runtime_error(name+" must be exactly 32 bytes / 64 hex");
    }
    uint256 ret;
    std::copy(vch.begin(), vch.end(), ret.begin());
    return ret;
}

static double ClampDifficulty(const StratumClient& client, double diff)
{
    if (client.m_mindiff > 0) {
        diff = client.m_mindiff;
    }
    diff = std::max(diff, 0.001);
    return diff;
}

static std::string GetExtraNonceRequest(StratumClient& client, const uint256& job_id)
{
    std::string ret;
    if (client.m_supports_extranonce) {
        const std::string k_extranonce_req = std::string()
            + "{"
            +     "\"id\":";
        const std::string k_extranonce_req2 = std::string()
            +     ","
            +     "\"method\":\"mining.set_extranonce\","
            +     "\"params\":["
            +         "\"";
        const std::string k_extranonce_req3 = std::string()
            +            "\"," // extranonce1
            +         "4"      // extranonce2.size()
            +     "]"
            + "}"
            + "\n";

        ret = k_extranonce_req
            + strprintf("%d", client.m_nextid++)
            + k_extranonce_req2
            + HexStr(client.ExtraNonce1(job_id))
            + k_extranonce_req3;
    }
    return ret;
}

void CustomizeWork(const StratumClient& client, const StratumWork& current_work, const CFreicoinAddress& addr, const std::vector<unsigned char>& extranonce1, const std::vector<unsigned char>& extranonce2, CMutableTransaction& cb, CMutableTransaction& bf, std::vector<uint256>& cb_branch)
{
    if (current_work.GetBlock().vtx.empty()) {
        const std::string msg = strprintf("%s: no transactions in block template; unable to submit work", __func__);
        LogPrint("stratum", "%s\n", msg);
        throw std::runtime_error(msg);
    }
    cb = CMutableTransaction(current_work.GetBlock().vtx[0]);
    if (cb.vin.size() != 1) {
        const std::string msg = strprintf("%s: unexpected number of inputs; is this even a coinbase transaction?", __func__);
        LogPrint("stratum", "%s\n", msg);
        throw std::runtime_error(msg);
    }
    std::vector<unsigned char> nonce(extranonce1);
    if ((nonce.size() + extranonce2.size()) != 12) {
        const std::string msg = strprintf("%s: unexpected combined nonce length: extranonce1(%d) + extranonce2(%d) != 12; unable to submit work", __func__, nonce.size(), extranonce2.size());
        LogPrint("stratum", "%s\n", msg);
        throw std::runtime_error(msg);
    }
    nonce.insert(nonce.end(), extranonce2.begin(),
                              extranonce2.end());
    if (cb.vin.empty()) {
        const std::string msg = strprintf("%s: first transaction is missing coinbase input; unable to customize work to miner", __func__);
        LogPrint("stratum", "%s\n", msg);
        throw std::runtime_error(msg);
    }
    cb.vin[0].scriptSig =
           CScript()
        << cb.lock_height
        << nonce;
    if (current_work.m_aux_hash2) {
        cb.vin[0].scriptSig.insert(cb.vin[0].scriptSig.end(),
                                   current_work.m_aux_hash2->begin(),
                                   current_work.m_aux_hash2->end());
    } else {
        if (cb.vout.empty()) {
            const std::string msg = strprintf("%s: coinbase transaction is missing outputs; unable to customize work to miner", __func__);
            LogPrint("stratum", "%s\n", msg);
            throw std::runtime_error(msg);
        }
        if (cb.vout[0].scriptPubKey == (CScript() << OP_FALSE)) {
            cb.vout[0].scriptPubKey =
                GetScriptForDestination(addr.Get());
        }
    }

    cb_branch = current_work.m_cb_branch;
    if (!current_work.m_aux_hash2 && current_work.m_is_witness_enabled) {
        bf = CMutableTransaction(current_work.GetBlock().vtx.back());
        UpdateSegwitCommitment(current_work, cb, bf, cb_branch);
    }
}

uint256 CustomizeCommitHash(const StratumClient& client, const CFreicoinAddress& addr, const uint256& job_id, const StratumWork& current_work, const uint256& secret)
{
    CMutableTransaction cb, bf;
    std::vector<uint256> cb_branch;
    static const std::vector<unsigned char> dummy(4, 0x00); // extranonce2
    CustomizeWork(client, current_work, addr, client.ExtraNonce1(job_id), dummy, cb, bf, cb_branch);

    CMutableTransaction cb2(cb);
    cb2.vin[0].scriptSig = CScript();
    cb2.vin[0].nSequence = 0;

    const AuxProofOfWork& aux_pow = current_work.GetBlock().m_aux_pow;

    CBlockHeader blkhdr;
    blkhdr.nVersion = aux_pow.m_commit_version;
    blkhdr.hashPrevBlock = current_work.GetBlock().hashPrevBlock;
    blkhdr.hashMerkleRoot = ComputeMerkleRootFromBranch(cb2.GetHash(), cb_branch, 0);
    blkhdr.nTime = aux_pow.m_commit_time;
    blkhdr.nBits = aux_pow.m_commit_bits;
    blkhdr.nNonce = aux_pow.m_commit_nonce;
    uint256 hash = blkhdr.GetHash();

    MerkleHash_Sha256Midstate(hash, hash, secret);
    return hash;
}

std::string GetWorkUnit(StratumClient& client)
{
    LOCK(cs_main);

    if (vNodes.empty() && !Params().MineBlocksOnDemand()) {
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Freicoin is not connected!");
    }

    if (IsInitialBlockDownload()) {
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Freicoin is downloading blocks...");
    }

    if (!client.m_authorized && client.m_aux_addr.empty()) {
        throw JSONRPCError(RPC_INVALID_REQUEST, "Stratum client not authorized.  Use mining.authorize first, with a Freicoin address as the username.");
    }

    static CBlockIndex* tip = NULL;
    static uint256 job_id;
    static unsigned int transactions_updated_last = 0;
    static int64_t last_update_time = 0;

    // When merge-mining is active, finding a block is a two-stage process.
    // First the auxiliary proof-of-work is solved, which requires constructing
    // a fake bitcoin block which commits to our Freicoin block.  Then the
    // coinbase is updated to commit to the auxiliary proof-of-work solution and
    // the native proof-of-work is solved.
    if (half_solved_work && (tip != chainActive.Tip() || !work_templates.count(*half_solved_work))) {
        half_solved_work = boost::none;
    }

    if (half_solved_work) {
        job_id = *half_solved_work;
    } else
    if (tip != chainActive.Tip() || (mempool.GetTransactionsUpdated() != transactions_updated_last && (GetTime() - last_update_time) > 5) || !work_templates.count(job_id))
    {
        CBlockIndex *tip_new = chainActive.Tip();
        const CScript script = CScript() << OP_FALSE;
        CBlockTemplate *new_work = BlockAssembler(Params()).CreateNewBlock(script);
        if (!new_work) {
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
        }
        // So that block.GetHash() is correct
        new_work->block.hashMerkleRoot = BlockMerkleRoot(new_work->block);

        job_id = new_work->block.GetHash();
        work_templates[job_id] = StratumWork(*new_work, !new_work->block.vtx[0].wit.IsEmpty());
        tip = tip_new;

        transactions_updated_last = mempool.GetTransactionsUpdated();
        last_update_time = GetTime();

        delete new_work;
        new_work = NULL;

        LogPrint("stratum", "New stratum block template (%d total): %s\n", work_templates.size(), HexStr(job_id.begin(), job_id.end()));

        // Remove any old templates
        std::vector<uint256> old_job_ids;
        boost::optional<uint256> oldest_job_id = boost::none;
        uint32_t oldest_job_nTime = last_update_time;
        for (const auto& work_template : work_templates) {
            // If, for whatever reason the new work was generated with
            // an old nTime, don't erase it!
            if (work_template.first == job_id) {
                continue;
            }
            // Build a list of outdated work units to free.
            if (work_template.second.GetBlock().nTime < (last_update_time - 900)) {
                old_job_ids.push_back(work_template.first);
            }
            // Track the oldest work unit, in case we have too much
            // recent work.
            if (work_template.second.GetBlock().nTime <= oldest_job_nTime) {
                oldest_job_id = work_template.first;
                oldest_job_nTime = work_template.second.GetBlock().nTime;
            }
        }
        // Remove all outdated work.
        for (const auto& old_job_id : old_job_ids) {
            work_templates.erase(old_job_id);
            LogPrint("stratum", "Removed outdated stratum block template (%d total): %s\n", work_templates.size(), HexStr(old_job_id.begin(), old_job_id.end()));
        }
        // Remove the oldest work unit if we're still over the maximum
        // number of stored work templates.
        if (work_templates.size() > 30 && oldest_job_id) {
            work_templates.erase(oldest_job_id.get());
            LogPrint("stratum", "Removed oldest stratum block template (%d total): %s\n", work_templates.size(), HexStr(oldest_job_id.get().begin(), oldest_job_id.get().end()));
        }
    }

    StratumWork& current_work = work_templates[job_id];

    if (client.m_supports_aux && !current_work.GetBlock().m_aux_pow.IsNull() && !current_work.m_aux_hash2 && !client.m_aux_addr.empty()) {
        const AuxProofOfWork& aux_pow = current_work.GetBlock().m_aux_pow;

        CHashWriter secret_writer(SER_GETHASH, PROTOCOL_VERSION);
        secret_writer << aux_pow.m_secret_lo;
        secret_writer << aux_pow.m_secret_hi;
        uint256 secret = secret_writer.GetHash();

        UniValue commits(UniValue::VOBJ);
        for (const auto& addr : client.m_aux_addr) {
            uint256 hash = CustomizeCommitHash(client, addr, job_id, current_work, secret);
            commits.push_back(Pair(addr.ToString(), HexStr(hash.begin(), hash.end())));
        }

        UniValue params(UniValue::VARR);
        params.push_back(HexStr(job_id.begin(), job_id.end()));
        params.push_back(commits);

        unsigned char bias = current_work.GetBlock().GetBias();
        uint32_t bits = aux_pow.m_commit_bits;
        bits += static_cast<uint32_t>(bias / 8) << 24;
        bias = bias % 8;
        params.push_back(HexInt4(bits));
        params.push_back(UniValue((int)bias));

        params.push_back(UniValue(client.m_last_tip != tip));
        client.m_last_tip = tip;
        client.m_second_stage = false;

        UniValue mining_aux_notify(UniValue::VOBJ);
        mining_aux_notify.push_back(Pair("params", params));
        mining_aux_notify.push_back(Pair("id", client.m_nextid++));
        mining_aux_notify.push_back(Pair("method", "mining.aux.notify"));

        return mining_aux_notify.write() + '\n';
    }

    CBlockIndex tmp_index;
    if (!current_work.GetBlock().m_aux_pow.IsNull() && !current_work.m_aux_hash2) {
        // Auxiliary proof-of-work difficulty
        tmp_index.nBits = current_work.GetBlock().m_aux_pow.m_commit_bits;
    } else {
        // Native proof-of-work difficulty
        tmp_index.nBits = current_work.GetBlock().nBits;
    }
    double diff = ClampDifficulty(client, GetDifficulty(&tmp_index));

    UniValue set_difficulty(UniValue::VOBJ);
    set_difficulty.push_back(Pair("id", client.m_nextid++));
    set_difficulty.push_back(Pair("method", "mining.set_difficulty"));
    UniValue set_difficulty_params(UniValue::VARR);
    set_difficulty_params.push_back(UniValue(diff));
    set_difficulty.push_back(Pair("params", set_difficulty_params));

    CMutableTransaction cb, bf;
    std::vector<uint256> cb_branch;
    {
        static const std::vector<unsigned char> dummy(4, 0x00); // extranonce2
        CustomizeWork(client, current_work, client.m_addr, client.ExtraNonce1(job_id), dummy, cb, bf, cb_branch);
    }

    CBlockHeader blkhdr;
    if (!current_work.GetBlock().m_aux_pow.IsNull() && !current_work.m_aux_hash2) {
        // Setup auxiliary proof-of-work
        const AuxProofOfWork& aux_pow = current_work.GetBlock().m_aux_pow;

        CMutableTransaction cb2(cb);
        cb2.vin[0].scriptSig = CScript();
        cb2.vin[0].nSequence = 0;

        blkhdr.nVersion = aux_pow.m_commit_version;
        blkhdr.hashPrevBlock = current_work.GetBlock().hashPrevBlock;
        blkhdr.hashMerkleRoot = ComputeMerkleRootFromBranch(cb2.GetHash(), cb_branch, 0);
        blkhdr.nTime = aux_pow.m_commit_time;
        blkhdr.nBits = aux_pow.m_commit_bits;
        blkhdr.nNonce = aux_pow.m_commit_nonce;
        uint256 hash = blkhdr.GetHash();

        {
            CHashWriter secret(SER_GETHASH, PROTOCOL_VERSION);
            secret << aux_pow.m_secret_lo;
            secret << aux_pow.m_secret_hi;
            MerkleHash_Sha256Midstate(hash, hash, secret.GetHash());
        }

        hash = ComputeMerkleMapRootFromBranch(hash, aux_pow.m_commit_branch, Params().GetConsensus().aux_pow_path);

        {
            CSHA256 midstate(aux_pow.m_midstate_hash.begin(),
                             &aux_pow.m_midstate_buffer[0],
                             aux_pow.m_midstate_length << 3);
            // Write the commitment root hash.
            midstate.Write(hash.begin(), 32);
            // Write the commitment identifier.
            static const std::array<unsigned char, 4> id
                = { 0x4b, 0x4a, 0x49, 0x48 };
            midstate.Write(id.begin(), 4);
            // Write the transaction's nLockTime field.
            CDataStream lock_time(SER_GETHASH, PROTOCOL_VERSION);
            lock_time << aux_pow.m_aux_lock_time;
            midstate.Write((const unsigned char*)&lock_time[0], lock_time.size());
            // Double SHA-256.
            midstate.Finalize(hash.begin());
            CSHA256()
                .Write(hash.begin(), 32)
                .Finalize(hash.begin());
        }

        cb_branch.resize(1);
        cb_branch[0] = hash;
        blkhdr.nVersion = aux_pow.m_aux_version;
        blkhdr.hashPrevBlock = aux_pow.m_aux_hash_prev_block;
        blkhdr.nTime = current_work.GetBlock().nTime;
        blkhdr.nBits = aux_pow.m_aux_bits;
    }

    else {
        // Setup native proof-of-work
        blkhdr.nVersion = current_work.GetBlock().nVersion;
        blkhdr.hashPrevBlock = current_work.GetBlock().hashPrevBlock;
        blkhdr.nTime = current_work.GetBlock().nTime;
        blkhdr.nBits = current_work.GetBlock().nBits;
    }

    CDataStream ds(SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
    ds << CTransaction(cb);
    if (ds.size() < (4 + 1 + 32 + 4 + 1)) {
        throw std::runtime_error("Serialized transaction is too small to be parsed.  Is this even a coinbase?");
    }
    size_t pos = 4 + 1 + 32 + 4 + 1 + ds[4+1+32+4] - (current_work.m_aux_hash2? 32: 0);
    if (ds.size() < pos) {
        throw std::runtime_error("Customized coinbase transaction does not contain extranonce field at expected location.");
    }

    std::string cb1 = HexStr(&ds[0], &ds[pos-4-8]);
    std::string cb2 = HexStr(&ds[pos], &ds[ds.size()]);

    UniValue params(UniValue::VARR);
    params.push_back(HexStr(job_id.begin(), job_id.end()));
    // For reasons of who-the-heck-knows-why, stratum byte-swaps each
    // 32-bit chunk of the hashPrevBlock.
    uint256 hashPrevBlock(blkhdr.hashPrevBlock);
    for (int i = 0; i < 256/32; ++i) {
        ((uint32_t*)hashPrevBlock.begin())[i] = bswap_32(
        ((uint32_t*)hashPrevBlock.begin())[i]);
    }
    params.push_back(HexStr(hashPrevBlock.begin(), hashPrevBlock.end()));
    params.push_back(cb1);
    params.push_back(cb2);

    UniValue branch(UniValue::VARR);
    for (const auto& hash : cb_branch) {
        branch.push_back(HexStr(hash.begin(), hash.end()));
    }
    params.push_back(branch);

    if (!current_work.m_aux_hash2) {
        int64_t delta = UpdateTime(&blkhdr, Params().GetConsensus(), tip);
        LogPrint("stratum", "Updated the timestamp of block template by %d seconds\n", delta);
    }

    params.push_back(HexInt4(blkhdr.nVersion));
    params.push_back(HexInt4(blkhdr.nBits));
    params.push_back(HexInt4(blkhdr.nTime));
    params.push_back(UniValue((client.m_last_tip != tip)
                           || (client.m_second_stage != bool(current_work.m_aux_hash2))));
    client.m_last_tip = tip;
    client.m_second_stage = bool(current_work.m_aux_hash2);

    UniValue mining_notify(UniValue::VOBJ);
    mining_notify.push_back(Pair("params", params));
    mining_notify.push_back(Pair("id", client.m_nextid++));
    mining_notify.push_back(Pair("method", "mining.notify"));

    return GetExtraNonceRequest(client, job_id)
         + set_difficulty.write() + "\n"
         + mining_notify.write()  + "\n";
}

bool SubmitBlock(StratumClient& client, const uint256& job_id, const StratumWork& current_work, const std::vector<unsigned char>& extranonce1, const std::vector<unsigned char>& extranonce2, boost::optional<uint32_t> nVersion, uint32_t nTime, uint32_t nNonce)
{
    if (extranonce1.size() != 8) {
        std::string msg = strprintf("extranonce1 is wrong length (received %d bytes; expected %d bytes", extranonce2.size(), 8);
        LogPrint("stratum", msg.data());
        throw JSONRPCError(RPC_INVALID_PARAMETER, msg);
    }
    if (extranonce2.size() != 4) {
        std::string msg = strprintf("%s: extranonce2 is wrong length (received %d bytes; expected %d bytes", __func__, extranonce2.size(), 4);
        LogPrint("stratum", msg.data());
        throw JSONRPCError(RPC_INVALID_PARAMETER, msg);
    }

    CMutableTransaction cb, bf;
    std::vector<uint256> cb_branch;
    CustomizeWork(client, current_work, client.m_addr, extranonce1, extranonce2, cb, bf, cb_branch);

    bool res = false;
    if (!current_work.GetBlock().m_aux_pow.IsNull() && !current_work.m_aux_hash2) {
        // Check auxiliary proof-of-work
        uint32_t version = current_work.GetBlock().m_aux_pow.m_aux_version;
        if (nVersion && client.m_version_rolling_mask) {
            version = (version & ~client.m_version_rolling_mask)
                    | (*nVersion & client.m_version_rolling_mask);
        } else if (nVersion) {
            version = *nVersion;
        }

        CMutableTransaction cb2(cb);
        cb2.vin[0].scriptSig = CScript();
        cb2.vin[0].nSequence = 0;

        CBlockHeader blkhdr(current_work.GetBlock());
        blkhdr.m_aux_pow.m_commit_hash_merkle_root = ComputeMerkleRootFromBranch(cb2.GetHash(), cb_branch, 0);
        blkhdr.m_aux_pow.m_aux_branch.resize(1);
        blkhdr.m_aux_pow.m_aux_branch[0] = cb.GetHash();
        blkhdr.m_aux_pow.m_aux_num_txns = 2;
        blkhdr.nTime = nTime;
        blkhdr.m_aux_pow.m_aux_nonce = nNonce;
        blkhdr.m_aux_pow.m_aux_version = version;

        const Consensus::Params& params = Params().GetConsensus();
        res = CheckAuxiliaryProofOfWork(blkhdr, params);
        auto aux_hash = blkhdr.GetAuxiliaryHash(params);
        if (res) {
            LogPrintf("GOT AUXILIARY BLOCK!!! by %s: %s, %s\n", client.m_addr.ToString(), aux_hash.first.ToString(), aux_hash.second.ToString());
            blkhdr.hashMerkleRoot = ComputeMerkleRootFromBranch(cb.GetHash(), cb_branch, 0);
            uint256 new_job_id = blkhdr.GetHash();
            work_templates[new_job_id] = current_work;
            StratumWork& new_work = work_templates[new_job_id];
            new_work.GetBlock().vtx[0] = CTransaction(cb);
            if (new_work.m_is_witness_enabled) {
                new_work.GetBlock().vtx.back() = CTransaction(bf);
            }
            new_work.GetBlock().hashMerkleRoot = BlockMerkleRoot(new_work.GetBlock(), nullptr);
            new_work.m_cb_branch = cb_branch;
            new_work.GetBlock().m_aux_pow.m_commit_hash_merkle_root = blkhdr.m_aux_pow.m_commit_hash_merkle_root;
            new_work.GetBlock().m_aux_pow.m_aux_branch = blkhdr.m_aux_pow.m_aux_branch;
            new_work.GetBlock().m_aux_pow.m_aux_num_txns = blkhdr.m_aux_pow.m_aux_num_txns;
            new_work.GetBlock().nTime = nTime;
            new_work.GetBlock().m_aux_pow.m_aux_nonce = nNonce;
            new_work.GetBlock().m_aux_pow.m_aux_version = version;
            new_work.m_aux_hash2 = aux_hash.second;
            if (new_job_id != new_work.GetBlock().GetHash()) {
                throw std::runtime_error("First-stage hash does not match expected value.");
            }
            half_solved_work = new_job_id;
        } else {
            LogPrintf("NEW AUXILIARY SHARE!!! by %s: %s, %s\n", client.m_addr.ToString(), aux_hash.first.ToString(), aux_hash.second.ToString());
        }
    }

    else {
        // Check native proof-of-work
        uint32_t version = current_work.GetBlock().nVersion;
        if (nVersion && client.m_version_rolling_mask) {
            version = (version & ~client.m_version_rolling_mask)
                    | (*nVersion & client.m_version_rolling_mask);
        } else if (nVersion) {
            version = *nVersion;
        }

        if (!current_work.GetBlock().m_aux_pow.IsNull() && nTime != current_work.GetBlock().nTime) {
            LogPrintf("Error: miner %s returned altered nTime value for native proof-of-work; nTime-rolling is not supported\n", client.m_addr.ToString());
            throw JSONRPCError(RPC_INVALID_PARAMETER, "nTime-rolling is not supported");
        }

        CBlockHeader blkhdr;
        blkhdr.nVersion = version;
        blkhdr.hashPrevBlock = current_work.GetBlock().hashPrevBlock;
        blkhdr.hashMerkleRoot = ComputeMerkleRootFromBranch(cb.GetHash(), cb_branch, 0);
        blkhdr.nTime = nTime;
        blkhdr.nBits = current_work.GetBlock().nBits;
        blkhdr.nNonce = nNonce;

        res = CheckProofOfWork(blkhdr, Params().GetConsensus());
        uint256 hash = blkhdr.GetHash();
        if (res) {
            LogPrintf("GOT BLOCK!!! by %s: %s\n", client.m_addr.ToString(), hash.ToString());
            CBlock block(current_work.GetBlock());
            block.vtx[0] = CTransaction(cb);
            if (!current_work.m_aux_hash2 && current_work.m_is_witness_enabled) {
                block.vtx.back() = CTransaction(bf);
            }
            block.nVersion = version;
            block.hashMerkleRoot = BlockMerkleRoot(block);
            block.nTime = nTime;
            block.nNonce = nNonce;
            CValidationState state;
            res = ProcessNewBlock(state, Params(), NULL, &block, true, NULL, false);
        } else {
            LogPrintf("NEW SHARE!!! by %s: %s\n", client.m_addr.ToString(), hash.ToString());
        }
    }

    if (res) {
        client.m_send_work = true;
    }

    return res;
}

bool SubmitAuxiliaryBlock(StratumClient& client, const CFreicoinAddress& addr, const uint256& job_id, const StratumWork& current_work, CBlockHeader& blkhdr)
{
    CMutableTransaction cb, bf;
    std::vector<uint256> cb_branch;
    static const std::vector<unsigned char> dummy(4, 0x00); // extranonce2
    CustomizeWork(client, current_work, addr, client.ExtraNonce1(job_id), dummy, cb, bf, cb_branch);

    CMutableTransaction cb2(cb);
    cb2.vin[0].scriptSig = CScript();
    cb2.vin[0].nSequence = 0;

    blkhdr.m_aux_pow.m_commit_hash_merkle_root = ComputeMerkleRootFromBranch(cb2.GetHash(), cb_branch, 0);

    const Consensus::Params& params = Params().GetConsensus();
    auto aux_hash = blkhdr.GetAuxiliaryHash(params);
    if (!CheckAuxiliaryProofOfWork(blkhdr, params)) {
        LogPrintf("NEW AUXILIARY SHARE!!! by %s: %s, %s\n", addr.ToString(), aux_hash.first.ToString(), aux_hash.second.ToString());
        return false;
    }

    LogPrintf("GOT AUXILIARY BLOCK!!! by %s: %s, %s\n", addr.ToString(), aux_hash.first.ToString(), aux_hash.second.ToString());
    blkhdr.hashMerkleRoot = ComputeMerkleRootFromBranch(cb.GetHash(), cb_branch, 0);

    uint256 new_job_id = blkhdr.GetHash();
    work_templates[new_job_id] = current_work;
    StratumWork& new_work = work_templates[new_job_id];
    new_work.GetBlock().vtx[0] = CTransaction(cb);
    if (new_work.m_is_witness_enabled) {
        new_work.GetBlock().vtx.back() = CTransaction(bf);
    }
    new_work.GetBlock().hashMerkleRoot = BlockMerkleRoot(new_work.GetBlock(), nullptr);
    new_work.m_cb_branch = cb_branch;
    new_work.GetBlock().m_aux_pow = blkhdr.m_aux_pow;
    new_work.GetBlock().nTime = blkhdr.nTime;
    new_work.m_aux_hash2 = aux_hash.second;
    if (new_job_id != new_work.GetBlock().GetHash()) {
        throw std::runtime_error("First-stage hash does not match expected value.");
    }

    half_solved_work = new_job_id;
    client.m_send_work = true;

    return false;
}

void BoundParams(const std::string& method, const UniValue& params, size_t min, size_t max)
{
    if (params.size() < min) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s expects at least %d parameters; received %d", method, min, params.size()));
    }

    if (params.size() > max) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s receives no more than %d parameters; got %d", method, max, params.size()));
    }
}

UniValue stratum_mining_subscribe(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.subscribe");
    BoundParams(method, params, 0, 2);

    if (params.size() >= 1) {
        client.m_client = params[0].get_str();
        LogPrint("stratum", "Received subscription from client %s\n", client.m_client);
    }

    // params[1] is the subscription ID for reconnect, which we
    // currently do not support.

    UniValue msg(UniValue::VARR);

    // Some mining proxies (e.g. Nicehash) reject connections that don't send
    // a reasonable difficulty on first connection.  The actual value will be
    // overridden when the miner is authorized and work is delivered.  Oh, and
    // for reasons unknown it is sent in serialized float format rather than
    // as a numeric value...
    UniValue set_difficulty(UniValue::VARR);
    set_difficulty.push_back("mining.set_difficulty");
    set_difficulty.push_back("1e+06"); // Will be overriden by later
    msg.push_back(set_difficulty);     // work delivery messages.

    UniValue notify(UniValue::VARR);
    notify.push_back("mining.notify");
    notify.push_back("ae6812eb4cd7735a302a8a9dd95cf71f");
    msg.push_back(notify);

    UniValue ret(UniValue::VARR);
    ret.push_back(msg);
    // client.m_supports_extranonce is false, so the job_id isn't used.
    ret.push_back(HexStr(client.ExtraNonce1(uint256())));
    ret.push_back(UniValue(4)); // sizeof(extranonce2)

    return ret;
}

UniValue stratum_mining_authorize(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.authorize");
    BoundParams(method, params, 1, 2);

    std::string username = params[0].get_str();
    boost::trim(username);

    // params[1] is the client-provided password.  We do not perform
    // user authorization, so we ignore this value.

    double mindiff = 0.0;
    size_t pos = username.find('+');
    if (pos != std::string::npos) {
        // Extract the suffix and trim it
        std::string suffix(username, pos+1);
        boost::trim_left(suffix);
        // Extract the minimum difficulty request
        mindiff = boost::lexical_cast<double>(suffix);
        // Remove the '+' and everything after
        username.resize(pos);
        boost::trim_right(username);
    }

    CFreicoinAddress addr(username);

    if (!addr.IsValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid Freicoin address: %s", username));
    }

    client.m_addr = addr;
    client.m_mindiff = mindiff;
    client.m_authorized = true;

    client.m_send_work = true;

    LogPrintf("Authorized stratum miner %s from %s, mindiff=%f\n", addr.ToString(), client.GetPeer().ToString(), mindiff);

    return true;
}

UniValue stratum_mining_aux_authorize(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.aux.authorize");
    BoundParams(method, params, 1, 2);

    std::string username = params[0].get_str();
    boost::trim(username);

    // The second parameter is the password.  We don't actually do any
    // authorization, so we ignore the password field.

    size_t pos = username.find('+');
    if (pos != std::string::npos) {
        // Ignore suffix.
        username.resize(pos);
        boost::trim_right(username);
    }

    CFreicoinAddress addr(username);
    if (!addr.IsValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid Freicoin address: %s", username));
    }
    if (client.m_aux_addr.count(addr)) {
        LogPrint("stratum", "Client with address %s is already registered for stratum miner %s\n", addr.ToString(), client.GetPeer().ToString());
        return addr.ToString();
    }

    client.m_aux_addr.insert(addr);
    client.m_send_work = true;

    LogPrintf("Authorized client %s of stratum miner %s\n", addr.ToString(), client.GetPeer().ToString());

    return addr.ToString();
}

UniValue stratum_mining_aux_deauthorize(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.aux.deauthorize");
    BoundParams(method, params, 1, 1);

    std::string username = params[0].get_str();
    boost::trim(username);

    size_t pos = username.find('+');
    if (pos != std::string::npos) {
        // Ignore suffix.
        username.resize(pos);
        boost::trim_right(username);
    }

    CFreicoinAddress addr(username);
    if (!addr.IsValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid Freicoin address: %s", username));
    }
    if (!client.m_aux_addr.count(addr)) {
        LogPrint("stratum", "No client with address %s is currently registered for stratum miner %s\n", addr.ToString(), client.GetPeer().ToString());
        return false;
    }

    client.m_aux_addr.erase(addr);

    LogPrintf("Deauthorized client %s of stratum miner %s\n", addr.ToString(), client.GetPeer().ToString());

    return true;
}

UniValue stratum_mining_configure(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.configure");
    BoundParams(method, params, 2, 2);

    UniValue res(UniValue::VOBJ);

    UniValue extensions = params[0].get_array();
    UniValue config = params[1].get_obj();
    for (int i = 0; i < extensions.size(); ++i) {
        std::string name = extensions[i].get_str();

        if ("version-rolling" == name) {
            uint32_t mask = ParseHexInt4(find_value(config, "version-rolling.mask"), "version-rolling.mask");
            size_t min_bit_count = find_value(config, "version-rolling.min-bit-count").get_int();
            client.m_version_rolling_mask = mask & 0x1fffe000;
            res.push_back(Pair("version-rolling", true));
            res.push_back(Pair("version-rolling.mask", HexInt4(client.m_version_rolling_mask)));
            LogPrint("stratum", "Received version rolling request from %s\n", client.GetPeer().ToString());
        }

        else {
            LogPrint("stratum", "Unrecognized stratum extension '%s' sent by %s\n", name, client.GetPeer().ToString());
        }
    }

    return res;
}

UniValue stratum_mining_submit(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.submit");
    BoundParams(method, params, 5, 7);
    // First parameter is the client username, which is ignored.

    uint256 job_id = ParseUInt256(params[1], "job_id");
    if (!work_templates.count(job_id)) {
        LogPrint("stratum", "Received completed share for unknown job_id : %s\n", HexStr(job_id.begin(), job_id.end()));
        return false;
    }
    StratumWork &current_work = work_templates[job_id];

    std::vector<unsigned char> extranonce2 = ParseHexV(params[2], "extranonce2");
    if (extranonce2.size() != 4) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("extranonce2 is wrong length (received %d bytes; expected %d bytes", extranonce2.size(), 4));
    }
    uint32_t nTime = ParseHexInt4(params[3], "nTime");
    uint32_t nNonce = ParseHexInt4(params[4], "nNonce");
    boost::optional<uint32_t> nVersion;
    if (params.size() > 5) {
        nVersion = ParseHexInt4(params[5], "nVersion");
    }
    std::vector<unsigned char> extranonce1;
    if (params.size() > 6) {
        extranonce1 = ParseHexV(params[6], "extranonce1");
        if (extranonce1.size() != 8) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Expected 8 bytes for extranonce1 field; received %d", extranonce1.size()));
        }
    } else {
        extranonce1 = client.ExtraNonce1(job_id);
    }

    SubmitBlock(client, job_id, current_work, extranonce1, extranonce2, nVersion, nTime, nNonce);

    return true;
}

UniValue stratum_mining_aux_submit(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.aux.submit");
    BoundParams(method, params, 14, 14);

    std::string username = params[0].get_str();
    boost::trim(username);

    size_t pos = username.find('+');
    if (pos != std::string::npos) {
        // Ignore suffix.
        username.resize(pos);
        boost::trim_right(username);
    }

    CFreicoinAddress addr(username);
    if (!addr.IsValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid Freicoin address: %s", username));
    }
    if (!client.m_aux_addr.count(addr)) {
        LogPrint("stratum", "No user with address %s is currently registered\n", addr.ToString());
    }

    uint256 job_id = ParseUInt256(params[1].get_str(), "job_id");
    if (!work_templates.count(job_id)) {
        LogPrint("stratum", "Received completed auxiliary share for unknown job_id : %s\n", HexStr(job_id.begin(), job_id.end()));
        return false;
    }
    StratumWork &current_work = work_templates[job_id];

    CBlockHeader blkhdr(current_work.GetBlock());
    AuxProofOfWork& aux_pow = blkhdr.m_aux_pow;

    const UniValue& commit_branch = params[2].get_array();
    aux_pow.m_commit_branch.clear();
    for (int i = 0; i < commit_branch.size(); ++i) {
        const UniValue& inner_hash_node = commit_branch[i].get_array();
        if (inner_hash_node.size() != 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "commit_branch has unexpected size; must be of the form [[int, uint256]...]");
        }
        int bits = inner_hash_node[0].get_int();
        if (bits < 0 || bits > 255) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "bits parameter within commit_branch does not fit in unsigned char");
        }
        uint256 hash = ParseUInt256(inner_hash_node[1], "commit_branch");
        aux_pow.m_commit_branch.emplace_back((unsigned char)bits, hash);
    }
    if (aux_pow.m_commit_branch.size() > MAX_AUX_POW_COMMIT_BRANCH_LENGTH) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "auxiliary proof-of-work Merkle map path is too long");
    }
    size_t nbits = 0;
    for (size_t idx = 0; idx < aux_pow.m_commit_branch.size(); ++idx) {
        ++nbits;
        nbits += aux_pow.m_commit_branch[idx].first;
    }
    if (nbits >= 256) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "auxiliary proof-of-work Merkle map path is greater than 256 bits");
    }

    aux_pow.m_midstate_hash = ParseUInt256(params[3], "midstate_hash");
    if (!params[4].get_str().empty()) {
        aux_pow.m_midstate_buffer = ParseHexV(params[4], "midstate_buffer");
    }
    if (aux_pow.m_midstate_buffer.size() >= 64) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "auxiliary proof-of-work midstate buffer is too large");
    }
    int64_t midstate_length = 0;
    try {
        midstate_length = params[5].get_int64();
    } catch (...) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "midstate_length is not an integer as expected");
    }
    if (midstate_length < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "midstate length cannot be negative");
    }
    if (midstate_length >= std::numeric_limits<uint32_t>::max()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "non-representable midstate length for auxiliary proof-of-work");
    }
    aux_pow.m_midstate_length = (uint32_t)midstate_length;
    if (aux_pow.m_midstate_buffer.size() != aux_pow.m_midstate_length % 64) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "auxiliary proof-of-work midstate buffer doesn't match anticipated length");
    }

    aux_pow.m_aux_lock_time = ParseHexInt4(params[6], "lock_time");

    const UniValue& aux_branch = params[7].get_array();
    aux_pow.m_aux_branch.clear();
    for (int i = 0; i < aux_branch.size(); ++i) {
        aux_pow.m_aux_branch.push_back(ParseUInt256(aux_branch[i], "aux_branch"));
    }
    if (aux_pow.m_aux_branch.size() > MAX_AUX_POW_BRANCH_LENGTH) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "auxiliary proof-of-work Merkle branch is too long");
    }
    int64_t num_txns = params[8].get_int64();
    if (num_txns < 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "number of transactions in auxiliary block cannot be less than one");
    }
    if (num_txns > std::numeric_limits<uint32_t>::max()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "non-representable number of transactions in auxiliary block");
    }
    aux_pow.m_aux_num_txns = (uint32_t)num_txns;

    aux_pow.m_aux_hash_prev_block = ParseUInt256(params[10], "hashPrevBlock");
    blkhdr.nTime = ParseHexInt4(params[11], "nTime");
    aux_pow.m_aux_bits = ParseHexInt4(params[12], "nBits");
    aux_pow.m_aux_nonce = ParseHexInt4(params[13], "nNonce");
    aux_pow.m_aux_version = ParseHexInt4(params[9], "nVersion");

    SubmitAuxiliaryBlock(client, addr, job_id, current_work, blkhdr);

    return true;
}

UniValue stratum_mining_aux_subscribe(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.aux.subscribe");
    BoundParams(method, params, 0, 0);

    client.m_supports_aux = true;

    UniValue ret(UniValue::VARR);
    const uint256& aux_pow_path = Params().GetConsensus().aux_pow_path;
    ret.push_back(HexStr(aux_pow_path.begin(), aux_pow_path.end()));
    ret.push_back(UniValue((int)MAX_AUX_POW_COMMIT_BRANCH_LENGTH));
    ret.push_back(UniValue((int)MAX_AUX_POW_BRANCH_LENGTH));

    return ret;
}

UniValue stratum_mining_extranonce_subscribe(StratumClient& client, const UniValue& params)
{
    const std::string method("mining.extranonce.subscribe");
    BoundParams(method, params, 0, 0);

    client.m_supports_extranonce = true;

    return true;
}

/** Callback to read from a stratum connection. */
static void stratum_read_cb(bufferevent *bev, void *ctx)
{
    evconnlistener *listener = (evconnlistener*)ctx;
    LOCK(cs_stratum);
    // Lookup the client record for this connection
    if (!subscriptions.count(bev)) {
        LogPrint("stratum", "Received read notification for unknown stratum connection 0x%x\n", (size_t)bev);
        return;
    }
    StratumClient& client = subscriptions[bev];
    // Get links to the input and output buffers
    evbuffer *input = bufferevent_get_input(bev);
    evbuffer *output = bufferevent_get_output(bev);
    // Process each line of input that we have received
    char *cstr = 0;
    size_t len = 0;
    while ((cstr = evbuffer_readln(input, &len, EVBUFFER_EOL_CRLF))) {
        std::string line(cstr, len);
        free(cstr);
        LogPrint("stratum", "Received stratum request from %s : %s\n", client.GetPeer().ToString(), line);

        JSONRequest jreq;
        std::string reply;
        try {
            // Parse request
            UniValue valRequest;
            if (!valRequest.read(line)) {
                // Not JSON; is this even a stratum miner?
                throw JSONRPCError(RPC_PARSE_ERROR, "Parse error");
            }
            if (!valRequest.isObject()) {
                // Not a JSON object; don't know what to do.
                throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");
            }
            if (valRequest.exists("result")) {
                // JSON-RPC reply.  Ignore.
                LogPrint("stratum", "Ignoring JSON-RPC response\n");
                continue;
            }
            jreq.parse(valRequest);

            // Dispatch to method handler
            UniValue result = NullUniValue;
            if (stratum_method_dispatch.count(jreq.strMethod)) {
                result = stratum_method_dispatch[jreq.strMethod](client, jreq.params);
            } else {
                throw JSONRPCError(RPC_METHOD_NOT_FOUND, strprintf("Method '%s' not found", jreq.strMethod));
            }

            // Compose reply
            reply = JSONRPCReply(result, NullUniValue, jreq.id);
        } catch (const UniValue& objError) {
            reply = JSONRPCReply(NullUniValue, objError, jreq.id);
        } catch (const std::exception& e) {
            reply = JSONRPCReply(NullUniValue, JSONRPCError(RPC_INTERNAL_ERROR, e.what()), jreq.id);
        }

        LogPrint("stratum", "Sending stratum response to %s : %s", client.GetPeer().ToString(), reply);
        if (evbuffer_add(output, reply.data(), reply.size())) {
            LogPrint("stratum", "Sending stratum response failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
        }
    }

    // If required, send new work to the client.
    if (client.m_send_work) {
        std::string data;
        try {
            data = GetWorkUnit(client);
        } catch (const UniValue& objError) {
            data = JSONRPCReply(NullUniValue, objError, NullUniValue);
        } catch (const std::exception& e) {
            data = JSONRPCReply(NullUniValue, JSONRPCError(RPC_INTERNAL_ERROR, e.what()), NullUniValue);
        }

        LogPrint("stratum", "Sending requested stratum work unit to %s : %s", client.GetPeer().ToString(), data);
        if (evbuffer_add(output, data.data(), data.size())) {
            LogPrint("stratum", "Sending stratum work unit failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
        }

        client.m_send_work = false;
    }
}

/** Callback to handle unrecoverable errors in a stratum link. */
static void stratum_event_cb(bufferevent *bev, short what, void *ctx)
{
    evconnlistener *listener = (evconnlistener*)ctx;
    LOCK(cs_stratum);
    // Fetch the return address for this connection, for the debug log.
    std::string from("UNKNOWN");
    if (!subscriptions.count(bev)) {
        LogPrint("stratum", "Received event notification for unknown stratum connection 0x%x\n", (size_t)bev);
        return;
    } else {
        from = subscriptions[bev].GetPeer().ToString();
    }
    // Report the reason why we are closing the connection.
    if (what & BEV_EVENT_ERROR) {
        LogPrint("stratum", "Error detected on stratum connection from %s\n", from);
    }
    if (what & BEV_EVENT_EOF) {
        LogPrint("stratum", "Remote disconnect received on stratum connection from %s\n", from);
    }
    // Remove the connection from our records, and tell libevent to
    // disconnect and free its resources.
    if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        LogPrint("stratum", "Closing stratum connection from %s\n", from);
        subscriptions.erase(bev);
        if (bev) {
            bufferevent_free(bev);
            bev = NULL;
        }
    }
}

/** Callback to accept a stratum connection. */
static void stratum_accept_conn_cb(evconnlistener *listener, evutil_socket_t fd, sockaddr *address, int socklen, void *ctx)
{
    LOCK(cs_stratum);
    // Parse the return address
    CService from;
    from.SetSockAddr(address);
    // Early address-based allow check
    if (!ClientAllowed(stratum_allow_subnets, from)) {
        evconnlistener_free(listener);
        LogPrint("stratum", "Rejected connection from disallowed subnet: %s\n", from.ToString());
        return;
    }
    // Should be the same as EventBase(), but let's get it the
    // official way.
    event_base *base = evconnlistener_get_base(listener);
    // Create a buffer for sending/receiving from this connection.
    bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    // Disable Nagle's algorithm, so that TCP packets are sent
    // immediately, even if it results in a small packet.
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(one));
    // Setup the read and event callbacks to handle receiving requests
    // from the miner and error handling.  A write callback isn't
    // needed because we're not sending enough data to fill buffers.
    bufferevent_setcb(bev, stratum_read_cb, NULL, stratum_event_cb, (void*)listener);
    // Enable bidirectional communication on the connection.
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    // Record the connection state
    subscriptions[bev] = StratumClient(listener, fd, bev, from);
    // Log the connection.
    LogPrint("stratum", "Accepted stratum connection from %s\n", from.ToString());
}

/** Setup the stratum connection listening services */
static bool StratumBindAddresses(event_base* base)
{
    int defaultPort = GetArg("-stratumport", BaseParams().StratumPort());
    std::vector<std::pair<std::string, uint16_t> > endpoints;

    // Determine what addresses to bind to
    if (!InitEndpointList("stratum", defaultPort, endpoints))
        return false;

    // Bind each addresses
    for (const auto& endpoint : endpoints) {
        LogPrint("stratum", "Binding stratum on address %s port %i\n", endpoint.first, endpoint.second);
        // Use CService to translate string -> sockaddr
        CService socket(CNetAddr(endpoint.first), endpoint.second);
        union {
            sockaddr     ipv4;
            sockaddr_in6 ipv6;
        } addr;
        socklen_t len = sizeof(addr);
        socket.GetSockAddr((sockaddr*)&addr, &len);
        // Setup an event listener for the endpoint
        evconnlistener *listener = evconnlistener_new_bind(base, stratum_accept_conn_cb, NULL, LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, (sockaddr*)&addr, len);
        // Only record successful binds
        if (listener) {
            bound_listeners[listener] = socket;
        } else {
            LogPrintf("Binding stratum on address %s port %i failed. (Reason: %d, '%s')\n", endpoint.first, endpoint.second, errno, evutil_socket_error_to_string(errno));
        }
    }

    return !bound_listeners.empty();
}

/** Watches for new blocks and send updated work to miners. */
static bool g_shutdown = false;
void BlockWatcher()
{
    boost::unique_lock<boost::mutex> lock(csBestBlock);
    boost::system_time checktxtime = boost::get_system_time();
    unsigned int txns_updated_last = 0;
    while (true) {
        checktxtime += boost::posix_time::seconds(15);
        if (!cvBlockChange.timed_wait(lock, checktxtime)) {
            // Timeout: Check to see if mempool was updated.
            unsigned int txns_updated_next = mempool.GetTransactionsUpdated();
            if (txns_updated_last == txns_updated_next)
                continue;
            txns_updated_last = txns_updated_next;
        }

        LOCK(cs_stratum);

        if (g_shutdown) {
            break;
        }

        // Either new block, or updated transactions.  Either way,
        // send updated work to miners.
        for (auto& subscription : subscriptions) {
            bufferevent* bev = subscription.first;
            evbuffer *output = bufferevent_get_output(bev);
            StratumClient& client = subscription.second;
            // Ignore clients that aren't authorized yet.
            if (!client.m_authorized && client.m_aux_addr.empty()) {
                continue;
            }
            // Ignore clients that are already working on the new block.
            // Typically this is just the miner that found the block, who was
            // immediately sent a work update.  This check avoids sending that
            // work notification again, moments later.  Due to race conditions
            // there could be more than one miner that have already received an
            // update, however.
            if (client.m_last_tip == chainActive.Tip()) {
                continue;
            }
            // Get new work
            std::string data;
            try {
                data = GetWorkUnit(client);
            } catch (const UniValue& objError) {
                data = JSONRPCReply(NullUniValue, objError, NullUniValue);
            } catch (const std::exception& e) {
                // Some sort of error.  Ignore.
                std::string msg = strprintf("Error generating updated work for stratum client: %s", e.what());
                LogPrint("stratum", "%s\n", msg);
                data = JSONRPCReply(NullUniValue, JSONRPCError(RPC_INTERNAL_ERROR, msg), NullUniValue);
            }
            // Send the new work to the client
            LogPrint("stratum", "Sending updated stratum work unit to %s : %s", client.GetPeer().ToString(), data);
            if (evbuffer_add(output, data.data(), data.size())) {
                LogPrint("stratum", "Sending stratum work unit failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
            }
        }
    }
}

/** Configure the stratum server */
bool InitStratumServer()
{
    LOCK(cs_stratum);

    if (!InitSubnetAllowList("stratum", stratum_allow_subnets)) {
        LogPrint("stratum", "Unable to bind stratum server to an endpoint.\n");
        return false;
    }

    std::string strAllowed;
    for (const auto& subnet : stratum_allow_subnets) {
        strAllowed += subnet.ToString() + " ";
    }
    LogPrint("stratum", "Allowing stratum connections from: %s\n", strAllowed);

    event_base* base = EventBase();
    if (!base) {
        LogPrint("stratum", "No event_base object, cannot setup stratum server.\n");
        return false;
    }

    if (!StratumBindAddresses(base)) {
        LogPrintf("Unable to bind any endpoint for stratum server\n");
    } else {
        LogPrint("stratum", "Initialized stratum server\n");
    }

    stratum_method_dispatch["mining.subscribe"] = stratum_mining_subscribe;
    stratum_method_dispatch["mining.authorize"] = stratum_mining_authorize;
    stratum_method_dispatch["mining.configure"] = stratum_mining_configure;
    stratum_method_dispatch["mining.submit"]    = stratum_mining_submit;
    stratum_method_dispatch["mining.aux.submit"] = stratum_mining_aux_submit;
    stratum_method_dispatch["mining.aux.authorize"] =
        stratum_mining_aux_authorize;
    stratum_method_dispatch["mining.aux.deauthorize"] =
        stratum_mining_aux_deauthorize;
    stratum_method_dispatch["mining.aux.subscribe"] =
        stratum_mining_aux_subscribe;
    stratum_method_dispatch["mining.extranonce.subscribe"] =
        stratum_mining_extranonce_subscribe;

    // Start thread to wait for block notifications and send updated
    // work to miners.
    block_watcher_thread = boost::thread(BlockWatcher);

    return true;
}

/** Interrupt the stratum server connections */
void InterruptStratumServer()
{
    LOCK(cs_stratum);
    // Stop listening for connections on stratum sockets
    for (const auto& binding : bound_listeners) {
        LogPrint("stratum", "Interrupting stratum service on %s\n", binding.second.ToString());
        evconnlistener_disable(binding.first);
    }
    // Tell the block watching thread to stop
    g_shutdown = true;
}

/** Cleanup stratum server network connections and free resources. */
void StopStratumServer()
{
    LOCK(cs_stratum);
    /* Tear-down active connections. */
    for (const auto& subscription : subscriptions) {
        LogPrint("stratum", "Closing stratum server connection to %s due to process termination\n", subscription.second.GetPeer().ToString());
        bufferevent_free(subscription.first);
    }
    subscriptions.clear();
    /* Un-bind our listeners from their network interfaces. */
    for (const auto& binding : bound_listeners) {
        LogPrint("stratum", "Removing stratum server binding on %s\n", binding.second.ToString());
        evconnlistener_free(binding.first);
    }
    bound_listeners.clear();
    /* Free any allocated block templates. */
    work_templates.clear();
}

// End of File
