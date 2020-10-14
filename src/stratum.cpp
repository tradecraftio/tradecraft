// Copyright (c) 2020-2022 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stratum.h>

#include <base58.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <crypto/sha256.h>
#include <deploymentstatus.h>
#include <httpserver.h>
#include <key_io.h>
#include <logging.h>
#include <mergemine.h>
#include <netbase.h>
#include <net.h>
#include <node/context.h>
#include <node/miner.h>
#include <pow.h>
#include <rpc/blockchain.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>
#include <wallet/miner.h>

#include <univalue.h>

#include <algorithm> // for std::reverse
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <boost/algorithm/string.hpp> // for boost::trim
#include <boost/lexical_cast.hpp>

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
    CTxDestination m_addr;
    double m_mindiff;

    uint32_t m_version_rolling_mask;

    CBlockIndex* m_last_tip;
    bool m_send_work;

    bool m_supports_extranonce;

    StratumClient() : m_listener(0), m_socket(0), m_bev(0), m_nextid(0), m_authorized(false), m_mindiff(0.0), m_version_rolling_mask(0x00000000), m_last_tip(0), m_send_work(false), m_supports_extranonce(false) { GenSecret(); }
    StratumClient(evconnlistener* listener, evutil_socket_t socket, bufferevent* bev, CService from) : m_listener(listener), m_socket(socket), m_bev(bev), m_nextid(0), m_from(from), m_authorized(false), m_mindiff(0.0), m_version_rolling_mask(0x00000000), m_last_tip(0), m_send_work(false), m_supports_extranonce(false) { GenSecret(); }

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
    const CBlockIndex *m_prev_block_index;
    node::CBlockTemplate m_block_template;
    std::vector<uint256> m_cb_branch;
    bool m_is_witness_enabled;
    // The height is serialized in the coinbase string.  At the time the work is
    // customized, we have no need to keep the block chain context (pindexPrev),
    // so we store just the height value which is all we need.
    int m_height;

    StratumWork() : m_prev_block_index(0), m_is_witness_enabled(false), m_height(0) { };
    StratumWork(const CBlockIndex* prev_blocK_index, int height, const node::CBlockTemplate& block_template);

    CBlock& GetBlock()
      { return m_block_template.block; }
    const CBlock& GetBlock() const
      { return m_block_template.block; }
};

StratumWork::StratumWork(const CBlockIndex* prev_block_index, int height, const node::CBlockTemplate& block_template)
    : m_prev_block_index(prev_block_index)
    , m_block_template(block_template)
    , m_height(height)
{
    m_is_witness_enabled = DeploymentActiveAt(*prev_block_index, Params().GetConsensus(), Consensus::DEPLOYMENT_SEGWIT);
    if (!m_is_witness_enabled) {
        m_cb_branch = BlockMerkleBranch(m_block_template.block, 0);
    }
};

void UpdateSegwitCommitment(const StratumWork& current_work, CMutableTransaction& cb, CMutableTransaction& bf, std::vector<uint256>& cb_branch)
{
    CBlock block2(current_work.GetBlock());
    block2.vtx.front() = MakeTransactionRef(std::move(cb));
    block2.vtx.back() = MakeTransactionRef(std::move(bf));
    // Erase any existing commitments:
    int commitpos = -1;
    while ((commitpos = GetWitnessCommitmentIndex(block2)) != -1) {
        CMutableTransaction mtx(*block2.vtx[0]);
        mtx.vout.erase(mtx.vout.begin()+commitpos);
        block2.vtx[0] = MakeTransactionRef(std::move(mtx));
    }
    // Generate new commitment:
    GenerateCoinbaseCommitment(block2, current_work.m_prev_block_index, Params().GetConsensus());
    // Save results from temporary block structure:
    cb = CMutableTransaction(*block2.vtx.front());
    bf = CMutableTransaction(*block2.vtx.back());
    cb_branch = BlockMerkleBranch(block2, 0);
}

//! Critical seciton guarding access to any of the stratum global state
static RecursiveMutex cs_stratum;

//! Reference to the NodeContext for the process
static node::NodeContext* g_context;

//! List of subnets to allow stratum connections from
static std::vector<CSubNet> stratum_allow_subnets;

//! Bound stratum listening sockets
static std::map<evconnlistener*, CService> bound_listeners;

//! Active miners connected to us
static std::map<bufferevent*, StratumClient> subscriptions;

//! Mapping of stratum method names -> handlers
static std::map<std::string, std::function<UniValue(StratumClient&, const UniValue&)> > stratum_method_dispatch;

//! A mapping of job_id -> work templates
static std::map<uint256, StratumWork> work_templates;

//! A thread to watch for new blocks and send mining notifications
static std::thread block_watcher_thread;

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

static std::string GetExtraNonceRequest(StratumClient& client, const uint256& job_id) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
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

std::string GetWorkUnit(StratumClient& client) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
{
    if (!g_context) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error: Node context not found");
    }

    if (!Params().MineBlocksOnDemand() && !g_context->connman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }

    if (!Params().MineBlocksOnDemand() && g_context->connman->GetNodeCount(ConnectionDirection::Both) == 0) {
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, PACKAGE_NAME " is not connected!");
    }

    if (!g_context->chainman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error: Chain manager not found in node context");
    }

    if (g_context->chainman->ActiveChainstate().IsInitialBlockDownload()) {
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, PACKAGE_NAME " is in initial sync and waiting for blocks...");
    }

    if (!g_context->mempool) {
        throw JSONRPCError(RPC_CLIENT_MEMPOOL_DISABLED, "Error: Mempool disabled or instance not found");
    }

    if (!client.m_authorized) {
        throw JSONRPCError(RPC_INVALID_REQUEST, "Stratum client not authorized.  Use mining.authorize first, with a Bitcoin address as the username.");
    }

    static CBlockIndex* tip = NULL;
    static uint256 job_id;
    static unsigned int transactions_updated_last = 0;
    static int64_t last_update_time = 0;
    const CTxMemPool& mempool = *g_context->mempool;

    if (tip != g_context->chainman->ActiveChain().Tip() || (mempool.GetTransactionsUpdated() != transactions_updated_last && (GetTime() - last_update_time) > 5) || !work_templates.count(job_id))
    {
        CBlockIndex* tip_new = g_context->chainman->ActiveChain().Tip();
        const CScript script = CScript() << OP_FALSE;
        std::unique_ptr<node::CBlockTemplate> new_work;
        new_work = node::BlockAssembler(g_context->chainman->ActiveChainstate(), mempool, Params()).CreateNewBlock(script);
        if (!new_work) {
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
        }
        transactions_updated_last = mempool.GetTransactionsUpdated();
        last_update_time = GetTime();

        // Use the wallet to add a block-final transaction,
        // if there isn't one there already.
        if (!new_work->has_block_final_tx) {
            wallet::AddBlockFinalTransaction(*g_context, g_context->chainman->ActiveChainstate(), *new_work);
        }

        // So that block.GetHash() is correct
        new_work->block.hashMerkleRoot = BlockMerkleRoot(new_work->block);

        job_id = new_work->block.GetHash();
        work_templates[job_id] = StratumWork(tip_new, tip_new->nHeight + 1, *new_work);
        tip = tip_new;

        LogPrint(BCLog::STRATUM, "New stratum block template (%d total): %s\n", work_templates.size(), HexStr(job_id));

        // Remove any old templates
        std::vector<uint256> old_job_ids;
        std::optional<uint256> oldest_job_id = std::nullopt;
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
            LogPrint(BCLog::STRATUM, "Removed outdated stratum block template (%d total): %s\n", work_templates.size(), HexStr(old_job_id));
        }
        // Remove the oldest work unit if we're still over the maximum
        // number of stored work templates.
        if (work_templates.size() > 30 && oldest_job_id) {
            work_templates.erase(*oldest_job_id);
            LogPrint(BCLog::STRATUM, "Removed oldest stratum block template (%d total): %s\n", work_templates.size(), HexStr(*oldest_job_id));
        }
    }

    StratumWork& current_work = work_templates[job_id];

    CBlockIndex tmp_index;
    tmp_index.nBits = current_work.GetBlock().nBits;
    double diff = ClampDifficulty(client, GetDifficulty(&tmp_index));

    UniValue set_difficulty(UniValue::VOBJ);
    set_difficulty.pushKV("id", client.m_nextid++);
    set_difficulty.pushKV("method", "mining.set_difficulty");
    UniValue set_difficulty_params(UniValue::VARR);
    set_difficulty_params.push_back(UniValue(diff));
    set_difficulty.pushKV("params", set_difficulty_params);

    CMutableTransaction cb(*current_work.GetBlock().vtx[0]);
    auto nonce = client.ExtraNonce1(job_id);
    nonce.resize(nonce.size()+4, 0x00); // extranonce2
    cb.vin.front().scriptSig =
           CScript()
        << current_work.m_height
        << nonce;
    if (cb.vout.front().scriptPubKey == (CScript() << OP_FALSE)) {
        cb.vout.front().scriptPubKey =
            GetScriptForDestination(client.m_addr);
    }

    CDataStream ds(SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
    ds << CTransaction(cb);
    if (ds.size() < (4 + 1 + 32 + 4 + 1)) {
        throw std::runtime_error("Serialized transaction is too small to be parsed.  Is this even a coinbase?");
    }
    size_t pos = 4 + 1 + 32 + 4 + 1 + static_cast<size_t>(ds[4+1+32+4]);
    if (ds.size() < pos) {
        throw std::runtime_error("Customized coinbase transaction does not contain extranonce field at expected location.");
    }
    std::string cb1 = HexStr({&ds[0], pos-4-8});
    std::string cb2 = HexStr({&ds[pos], ds.size()-pos});

    UniValue params(UniValue::VARR);
    params.push_back(HexStr(job_id));
    // For reasons of who-the-heck-knows-why, stratum byte-swaps each
    // 32-bit chunk of the hashPrevBlock.
    uint256 hashPrevBlock(current_work.GetBlock().hashPrevBlock);
    for (int i = 0; i < 256/32; ++i) {
        ((uint32_t*)hashPrevBlock.begin())[i] = bswap_32(
        ((uint32_t*)hashPrevBlock.begin())[i]);
    }
    params.push_back(HexStr(hashPrevBlock));
    params.push_back(cb1);
    params.push_back(cb2);

    std::vector<uint256> cb_branch = current_work.m_cb_branch;
    if (current_work.m_is_witness_enabled) {
        CMutableTransaction bf(*current_work.GetBlock().vtx.back());
        UpdateSegwitCommitment(current_work, cb, bf, cb_branch);
    }

    UniValue branch(UniValue::VARR);
    for (const auto& hash : cb_branch) {
        branch.push_back(HexStr(hash));
    }
    params.push_back(branch);

    CBlockHeader blkhdr(current_work.GetBlock());
    int64_t delta = node::UpdateTime(&blkhdr, Params().GetConsensus(), tip);
    LogPrint(BCLog::STRATUM, "Updated the timestamp of block template by %d seconds\n", delta);

    params.push_back(HexInt4(blkhdr.nVersion));
    params.push_back(HexInt4(blkhdr.nBits));
    params.push_back(HexInt4(blkhdr.nTime));
    params.push_back(UniValue(client.m_last_tip != tip));
    client.m_last_tip = tip;

    UniValue mining_notify(UniValue::VOBJ);
    mining_notify.pushKV("params", params);
    mining_notify.pushKV("id", client.m_nextid++);
    mining_notify.pushKV("method", "mining.notify");

    return GetExtraNonceRequest(client, job_id)
         + set_difficulty.write() + "\n"
         + mining_notify.write()  + "\n";
}

bool SubmitBlock(StratumClient& client, const uint256& job_id, const StratumWork& current_work, std::vector<unsigned char> extranonce2, uint32_t nTime, uint32_t nNonce, uint32_t nVersion) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
{
    if (current_work.GetBlock().vtx.empty()) {
        const std::string msg("SubmitBlock: no transactions in block template; unable to submit work");
        LogPrint(BCLog::STRATUM, "%s\n", msg);
        throw std::runtime_error(msg);
    }
    CMutableTransaction cb(*current_work.GetBlock().vtx.front());
    if (cb.vin.size() != 1) {
        const std::string msg("SubmitBlock: unexpected number of inputs; is this even a coinbase transaction?");
        LogPrint(BCLog::STRATUM, "%s\n", msg);
        throw std::runtime_error(msg);
    }
    auto nonce = client.ExtraNonce1(job_id);
    if ((nonce.size() + extranonce2.size()) != 12) {
        const std::string msg = strprintf("SubmitBlock: unexpected combined nonce length: extranonce1(%d) + extranonce2(%d) != 12; unable to submit work", nonce.size(), extranonce2.size());
        LogPrint(BCLog::STRATUM, "%s\n", msg);
        throw std::runtime_error(msg);
    }
    nonce.insert(nonce.end(), extranonce2.begin(),
                              extranonce2.end());
    if (cb.vin.empty()) {
        const std::string msg("SubmitBlock: first transaction is missing coinbase input; unable to customize work to miner");
        LogPrint(BCLog::STRATUM, "%s\n", msg);
        throw std::runtime_error(msg);
    }
    cb.vin.front().scriptSig =
           CScript()
        << current_work.m_height
        << nonce;
    if (cb.vout.empty()) {
        const std::string msg("SubmitBlock: coinbase transaction is missing outputs; unable to customize work to miner");
        LogPrint(BCLog::STRATUM, "%s\n", msg);
        throw std::runtime_error(msg);
    }
    if (cb.vout.front().scriptPubKey == (CScript() << OP_FALSE)) {
        cb.vout.front().scriptPubKey =
            GetScriptForDestination(client.m_addr);
    }

    CMutableTransaction bf(*current_work.GetBlock().vtx.back());
    std::vector<uint256> cb_branch = current_work.m_cb_branch;
    if (current_work.m_is_witness_enabled) {
        UpdateSegwitCommitment(current_work, cb, bf, cb_branch);
        LogPrint(BCLog::STRATUM, "Updated segwit commitment in coinbase.\n");
    }

    CBlockHeader blkhdr(current_work.GetBlock());
    blkhdr.nVersion = nVersion;
    blkhdr.hashMerkleRoot = ComputeMerkleRootFromBranch(cb.GetHash(), cb_branch, 0);
    blkhdr.nTime = nTime;
    blkhdr.nNonce = nNonce;

    bool res = false;
    uint256 hash = blkhdr.GetHash();
    if (CheckProofOfWork(hash, blkhdr.nBits, 0, Params().GetConsensus())) {
        LogPrintf("GOT BLOCK!!! by %s: %s\n", EncodeDestination(client.m_addr), hash.ToString());
        CBlock block(current_work.GetBlock());
        block.nVersion = nVersion;
        block.vtx.front() = MakeTransactionRef(std::move(cb));
        if (current_work.m_is_witness_enabled) {
            block.vtx.back() = MakeTransactionRef(std::move(bf));
        }
        block.hashMerkleRoot = BlockMerkleRoot(block);
        block.nTime = nTime;
        block.nNonce = nNonce;
        std::shared_ptr<const CBlock> pblock = std::make_shared<const CBlock>(block);
        if (!g_context) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error: Node context not found when submitting block");
        }
        if (!g_context->chainman) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error: Node chainman not found when submitting block");
        }
        res = g_context->chainman->ProcessNewBlock(Params(), pblock, true, NULL);
    } else {
        LogPrintf("NEW SHARE!!! by %s: %s\n", EncodeDestination(client.m_addr), hash.ToString());
    }

    if (res) {
        client.m_send_work = true;
    }

    return res;
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

UniValue stratum_mining_subscribe(StratumClient& client, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
{
    const std::string method("mining.subscribe");
    BoundParams(method, params, 0, 2);

    if (params.size() >= 1) {
        client.m_client = params[0].get_str();
        LogPrint(BCLog::STRATUM, "Received subscription from client %s\n", client.m_client);
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

UniValue stratum_mining_authorize(StratumClient& client, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
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

    CTxDestination addr = DecodeDestination(username);

    if (!IsValidDestination(addr)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid Bitcoin address: %s", username));
    }

    client.m_addr = addr;
    client.m_mindiff = mindiff;
    client.m_authorized = true;

    client.m_send_work = true;

    LogPrintf("Authorized stratum miner %s from %s, mindiff=%f\n", EncodeDestination(addr), client.GetPeer().ToString(), mindiff);

    return true;
}

UniValue stratum_mining_configure(StratumClient& client, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
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
            res.pushKV("version-rolling", true);
            res.pushKV("version-rolling.mask", HexInt4(client.m_version_rolling_mask));
            LogPrint(BCLog::STRATUM, "Received version rolling request from %s\n", client.GetPeer().ToString());
        }

        else {
            LogPrint(BCLog::STRATUM, "Unrecognized stratum extension '%s' sent by %s\n", name, client.GetPeer().ToString());
        }
    }

    return res;
}

UniValue stratum_mining_submit(StratumClient& client, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
{
    const std::string method("mining.submit");
    BoundParams(method, params, 5, 6);
    // First parameter is the client username, which is ignored.

    uint256 job_id = ParseUInt256(params[1], "job_id");
    if (!work_templates.count(job_id)) {
        LogPrint(BCLog::STRATUM, "Received completed share for unknown job_id : %s\n", HexStr(job_id));
        return false;
    }
    StratumWork &current_work = work_templates[job_id];

    std::vector<unsigned char> extranonce2 = ParseHexV(params[2], "extranonce2");
    if (extranonce2.size() != 4) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("extranonce2 is wrong length (received %d bytes; expected %d bytes", extranonce2.size(), 4));
    }
    uint32_t nTime = ParseHexInt4(params[3], "nTime");
    uint32_t nNonce = ParseHexInt4(params[4], "nNonce");
    uint32_t nVersion = current_work.GetBlock().nVersion;
    if (params.size() > 5) {
        uint32_t bits = ParseHexInt4(params[5], "nVersion");
        nVersion = (nVersion & ~client.m_version_rolling_mask)
                 | (bits & client.m_version_rolling_mask);
    }

    SubmitBlock(client, job_id, current_work, extranonce2, nTime, nNonce, nVersion);

    return true;
}

UniValue stratum_mining_extranonce_subscribe(StratumClient& client, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
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
        LogPrint(BCLog::STRATUM, "Received read notification for unknown stratum connection 0x%x\n", (size_t)bev);
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
        LogPrint(BCLog::STRATUM, "Received stratum request from %s : %s\n", client.GetPeer().ToString(), line);

        JSONRPCRequest jreq;
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
                LogPrint(BCLog::STRATUM, "Ignoring JSON-RPC response\n");
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

        LogPrint(BCLog::STRATUM, "Sending stratum response to %s : %s", client.GetPeer().ToString(), reply);
        if (evbuffer_add(output, reply.data(), reply.size())) {
            LogPrint(BCLog::STRATUM, "Sending stratum response failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
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

        LogPrint(BCLog::STRATUM, "Sending requested stratum work unit to %s : %s", client.GetPeer().ToString(), data);
        if (evbuffer_add(output, data.data(), data.size())) {
            LogPrint(BCLog::STRATUM, "Sending stratum work unit failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
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
        LogPrint(BCLog::STRATUM, "Received event notification for unknown stratum connection 0x%x\n", (size_t)bev);
        return;
    } else {
        from = subscriptions[bev].GetPeer().ToString();
    }
    // Report the reason why we are closing the connection.
    if (what & BEV_EVENT_ERROR) {
        LogPrint(BCLog::STRATUM, "Error detected on stratum connection from %s\n", from);
    }
    if (what & BEV_EVENT_EOF) {
        LogPrint(BCLog::STRATUM, "Remote disconnect received on stratum connection from %s\n", from);
    }
    // Remove the connection from our records, and tell libevent to
    // disconnect and free its resources.
    if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        LogPrint(BCLog::STRATUM, "Closing stratum connection from %s\n", from);
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
        LogPrint(BCLog::STRATUM, "Rejected connection from disallowed subnet: %s\n", from.ToString());
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
    LogPrint(BCLog::STRATUM, "Accepted stratum connection from %s\n", from.ToString());
}

/** Setup the stratum connection listening services */
static bool StratumBindAddresses(event_base* base, node::NodeContext& node) EXCLUSIVE_LOCKS_REQUIRED(cs_stratum)
{
    const ArgsManager& args = *Assert(node.args);
    int defaultPort = args.GetIntArg("-stratumport", BaseParams().StratumPort());
    std::vector<std::pair<std::string, uint16_t> > endpoints;

    // Determine what addresses to bind to
    if (!InitEndpointList("stratum", defaultPort, endpoints))
        return false;

    // Bind each addresses
    for (const auto& endpoint : endpoints) {
        LogPrint(BCLog::STRATUM, "Binding stratum on address %s port %i\n", endpoint.first, endpoint.second);
        // Use CService to translate string -> sockaddr
        CNetAddr netaddr;
        LookupHost(endpoint.first.c_str(), netaddr, true);
        CService socket(netaddr, endpoint.second);
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
static std::atomic<bool> g_shutdown = false;
void BlockWatcher()
{
    std::chrono::steady_clock::time_point checktxtime = std::chrono::steady_clock::now();
    unsigned int txns_updated_last = 0;
    while (true) {
        {
            WAIT_LOCK(g_best_block_mutex, lock);
            checktxtime += std::chrono::seconds(15);
            if (g_best_block_cv.wait_until(lock, checktxtime) == std::cv_status::timeout) {
                // Attempt to re-establish any connections that have been dropped.
                ReconnectToMergeMineEndpoints();

                // Timeout: Check to see if mempool was updated.
                unsigned int txns_updated_next = g_context->mempool ? g_context->mempool->GetTransactionsUpdated() : txns_updated_last;
                if (txns_updated_last == txns_updated_next)
                    continue;
                txns_updated_last = txns_updated_next;
            }
        }

        // Attempt to re-establish any connections that have been dropped.
        ReconnectToMergeMineEndpoints();

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
            if (!client.m_authorized) {
                continue;
            }
            // Ignore clients that are already working on the new block.
            // Typically this is just the miner that found the block, who was
            // immediately sent a work update.  This check avoids sending that
            // work notification again, moments later.  Due to race conditions
            // there could be more than one miner that have already received an
            // update, however.
            CBlockIndex* tip = nullptr;
            if (g_context && g_context->chainman) {
                tip = g_context->chainman->ActiveChain().Tip();
            }
            if (client.m_last_tip == tip) {
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
                LogPrint(BCLog::STRATUM, "%s\n", msg);
                data = JSONRPCReply(NullUniValue, JSONRPCError(RPC_INTERNAL_ERROR, msg), NullUniValue);
            }
            // Send the new work to the client
            LogPrint(BCLog::STRATUM, "Sending updated stratum work unit to %s : %s", client.GetPeer().ToString(), data);
            if (evbuffer_add(output, data.data(), data.size())) {
                LogPrint(BCLog::STRATUM, "Sending stratum work unit failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
            }
        }
    }
}

/** Configure the stratum server */
bool InitStratumServer(node::NodeContext& node)
{
    LOCK(cs_stratum);

    if (!InitSubnetAllowList("stratum", stratum_allow_subnets)) {
        LogPrint(BCLog::STRATUM, "Unable to bind stratum server to an endpoint.\n");
        return false;
    }

    std::string strAllowed;
    for (const auto& subnet : stratum_allow_subnets) {
        strAllowed += subnet.ToString() + " ";
    }
    LogPrint(BCLog::STRATUM, "Allowing stratum connections from: %s\n", strAllowed);

    event_base* base = EventBase();
    if (!base) {
        LogPrint(BCLog::STRATUM, "No event_base object, cannot setup stratum server.\n");
        return false;
    }

    if (!StratumBindAddresses(base, node)) {
        LogPrintf("Unable to bind any endpoint for stratum server\n");
    } else {
        LogPrint(BCLog::STRATUM, "Initialized stratum server\n");
    }

    g_context = &node;

    stratum_method_dispatch["mining.subscribe"] = stratum_mining_subscribe;
    stratum_method_dispatch["mining.authorize"] = stratum_mining_authorize;
    stratum_method_dispatch["mining.configure"] = stratum_mining_configure;
    stratum_method_dispatch["mining.submit"]    = stratum_mining_submit;
    stratum_method_dispatch["mining.extranonce.subscribe"] =
        stratum_mining_extranonce_subscribe;

    // Start thread to wait for block notifications and send updated
    // work to miners.
    block_watcher_thread = std::thread(BlockWatcher);

    return true;
}

/** Interrupt the stratum server connections */
void InterruptStratumServer()
{
    LOCK(cs_stratum);
    // Stop listening for connections on stratum sockets
    for (const auto& binding : bound_listeners) {
        LogPrint(BCLog::STRATUM, "Interrupting stratum service on %s\n", binding.second.ToString());
        evconnlistener_disable(binding.first);
    }
    // Tell the block watching thread to stop
    g_shutdown = true;
}

/** Cleanup stratum server network connections and free resources. */
void StopStratumServer()
{
    g_shutdown = true;
    if (block_watcher_thread.joinable()) {
        block_watcher_thread.join();
    }
    LOCK(cs_stratum);
    /* Tear-down active connections. */
    for (const auto& subscription : subscriptions) {
        LogPrint(BCLog::STRATUM, "Closing stratum server connection to %s due to process termination\n", subscription.second.GetPeer().ToString());
        bufferevent_free(subscription.first);
    }
    subscriptions.clear();
    /* Un-bind our listeners from their network interfaces. */
    for (const auto& binding : bound_listeners) {
        LogPrint(BCLog::STRATUM, "Removing stratum server binding on %s\n", binding.second.ToString());
        evconnlistener_free(binding.first);
    }
    bound_listeners.clear();
    /* Free any allocated block templates. */
    work_templates.clear();
}

// End of File
