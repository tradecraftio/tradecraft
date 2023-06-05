// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mergemine.h>

#include <chainparams.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <hash.h>
#include <logging.h>
#include <netbase.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <stratum.h>
#include <sync.h>
#include <uint256.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/thread.h>
#include <validation.h>

#include <univalue.h>

#include <string.h> // for memset

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <thread>
#include <vector>

// for argument-dependent lookup
using std::swap;

#include <boost/algorithm/string.hpp> // for boost::trim

#include <event2/event.h>
#include <event2/thread.h>
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

using node::NodeContext;

//! Reference to the NodeContext for the process
static NodeContext* g_context;

//! Event base
static event_base* base = nullptr;

//! The time (in seconds since the epoch) at which the merge-mining
//! coodination server was started.  Used for calculating uptime.
static int64_t start_time;

//! Mapping of alternative names to chain specifiers
std::map<std::string, ChainId> chain_names;

//! Mapping of chain specifiers to overriding default authorization credentials
std::map<ChainId, std::pair<std::string, std::string>> default_auths;

//! Merge-mining manager thread
static std::thread merge_mining_manager_thread;

struct AuxWorkServer {
    //! The name of the server, as specified in the config file.
    std::string name;
    //! The socket used as the return address to the server.
    CService socket;
    //! The libevent buffer used to communicate with the server.
    bufferevent* bev;
    //! The time at which the connection was established.
    int64_t connect_time;
    //! Last time a message was received from this server.
    int64_t last_recv_time;
    //! Last time a work unit was received from this server.
    int64_t last_work_time;
    //! Last time we submitted a solved share to this server.
    int64_t last_submit_time;
    //! Whether we have received the first two (ignorable) responses from the server.
    int idflags;
    //! The next id to use for a request.
    int nextid;
    //! The extranonce1 value to use for this server.
    std::vector<unsigned char> extranonce1;
    //! The size of the extranonce2 value to use for this server.
    int extranonce2_size;
    //! The chain specifier for the chain being merged-mined.
    ChainId aux_pow_path;
    //! Default authorization credentials, if any.
    std::optional<std::pair<std::string, std::string>> default_auth;
    //! Map response ids to the associated user.
    std::map<int, std::string> aux_auth_jreqid;
    //! Authentication credentials for users.
    std::map<std::string, std::string> aux_auth;
    //! Currently active work units.
    std::map<std::string, AuxWork> aux_work;
    //! The current difficulty.
    double diff;

    AuxWorkServer() : bev(nullptr), connect_time(GetTime()), last_recv_time(0), last_work_time(0), last_submit_time(0), idflags(0), nextid(0), extranonce2_size(0), diff(0.0) {}
    AuxWorkServer(const std::string& nameIn, const CService& socketIn, bufferevent* bevIn) : name(nameIn), socket(socketIn), bev(bevIn), connect_time(GetTime()), last_recv_time(0), last_work_time(0), last_submit_time(0), idflags(0), nextid(0), extranonce2_size(0), diff(0.0) {}
};

struct AuxServerDisconnect {
    //! The time at which communication with the server was lost.
    uint64_t timestamp;
    //! The name of the server, as specified in the config file.
    std::string name;

    AuxServerDisconnect() { timestamp = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()); }
    AuxServerDisconnect(const std::string& nameIn) : name(nameIn) { timestamp = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()); }
};

//! Mapping of stratum method names -> handlers
static std::map<std::string, std::function<UniValue(AuxWorkServer&, const UniValue&)> > stratum_method_dispatch;

//! Critical seciton guarding access to all of the merge-mining global state
RecursiveMutex cs_mergemine;

//! Auxiliary work servers for which we have yet to establish a connection,
//! or need to re-establish a connection.
static std::map<CService, AuxServerDisconnect> g_mergemine_noconn;

//! Connected auxiliary work servers.
static std::map<bufferevent*, AuxWorkServer> g_mergemine_conn;

//! An index mapping aux_pow_path to AuxWorkServer (via bev).
static std::map<ChainId, bufferevent*> g_mergemine;

// Forward declaration of stratum endpoint reconnection utility function.
static void LocalReconnectToMergeMineEndpoints();

//! A collection of second-stage work units to be solved, mapping chainid -> SecondStageWork.
static std::map<ChainId, SecondStageWork> g_second_stage;

static AuxWorkServer* GetServerFromChainId(const ChainId& chainid, const std::string& caller) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    if (!g_mergemine.count(chainid)) {
        LogPrint(BCLog::MERGEMINE, "%s: error: unrecognized chainid with no active connection: 0x%s\n", caller, HexStr(chainid));
        return nullptr;
    }
    bufferevent* bev = g_mergemine[chainid];

    if (!g_mergemine_conn.count(bev)) {
        // This should never happen!!
        LogPrintf("%s: error: currently no server record for bufferevent object; this should never happen!\n", caller);
        return nullptr;
    }
    return &g_mergemine_conn[bev];
}

static int SendAuxAuthorizeRequest(AuxWorkServer& server, const std::string& username, const std::string& password) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    LogPrintf("Authorizing aux-pow work on chain 0x%s through stratum+tcp://%s (%s) for client %s\n", HexStr(server.aux_pow_path), server.socket.ToStringAddrPort(), server.name, username);

    int id = server.nextid++;

    UniValue msg(UniValue::VOBJ);
    msg.pushKV("id", id);
    msg.pushKV("method", "mining.aux.authorize");
    UniValue params(UniValue::VARR);
    params.push_back(username);
    if (!password.empty()) {
        params.push_back(password);
    }
    msg.pushKV("params", params);

    std::string request = msg.write() + "\n";
    LogPrint(BCLog::MERGEMINE, "Sending stratum request to %s (%s) : %s", server.socket.ToStringAddrPort(), server.name, request);
    evbuffer *output = bufferevent_get_output(server.bev);
    if (evbuffer_add(output, request.data(), request.size())) {
        LogPrint(BCLog::MERGEMINE, "Sending stratum request failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
    }

    return id;
}

static int SendAuxSubmitRequest(AuxWorkServer& server, const std::string& address, const AuxWork& work, const AuxProof& proof) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    int id = server.nextid++;

    // Construct the mining.aux.submit message
    UniValue msg(UniValue::VOBJ);
    msg.pushKV("id", id);
    msg.pushKV("method", "mining.aux.submit");
    UniValue params(UniValue::VARR);
    params.push_back(address);
    params.push_back(work.job_id);
    UniValue path(UniValue::VARR);
    for (const auto& item : work.path) {
        UniValue tuple(UniValue::VARR);
        tuple.push_back((int)item.first);
        tuple.push_back(HexStr(item.second));
        path.push_back(tuple);
    }
    params.push_back(path);
    params.push_back(HexStr(proof.midstate_hash));
    params.push_back(HexStr(proof.midstate_buffer));
    params.push_back(UniValue(static_cast<uint64_t>(proof.midstate_length)));
    params.push_back(HexInt4(proof.lock_time));
    UniValue aux_branch(UniValue::VARR);
    for (const uint256& hash : proof.aux_branch) {
        aux_branch.push_back(HexStr(hash));
    }
    params.push_back(aux_branch);
    params.push_back(UniValue(static_cast<uint64_t>(proof.num_txns)));
    params.push_back(HexInt4(proof.nVersion));
    params.push_back(HexStr(proof.hashPrevBlock));
    params.push_back(HexInt4(proof.nTime));
    params.push_back(HexInt4(proof.nBits));
    params.push_back(HexInt4(proof.nNonce));
    msg.pushKV("params", params);

    // Send
    std::string request = msg.write() + "\n";
    LogPrint(BCLog::MERGEMINE, "Sending stratum request to %s (%s) : %s", server.socket.ToStringAddrPort(), server.name, request);
    evbuffer *output = bufferevent_get_output(server.bev);
    if (evbuffer_add(output, request.data(), request.size())) {
        LogPrint(BCLog::MERGEMINE, "Sending stratum request failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
    }

    return id;
}

static int SendSubmitRequest(AuxWorkServer& server, const std::string& address, const SecondStageWork& work, const SecondStageProof& proof) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    int id = server.nextid++;

    // Construct the mining.submit message
    UniValue msg(UniValue::VOBJ);
    msg.pushKV("id", id);
    msg.pushKV("method", "mining.submit");
    UniValue params(UniValue::VARR);
    params.push_back(address);
    params.push_back(work.job_id);
    params.push_back(HexStr(proof.extranonce2));
    params.push_back(HexInt4(proof.nTime));
    params.push_back(HexInt4(proof.nNonce));
    params.push_back(HexInt4(proof.nVersion));
    params.push_back(HexStr(proof.extranonce1));
    msg.pushKV("params", params);

    // Send
    std::string request = msg.write() + "\n";
    LogPrint(BCLog::MERGEMINE, "Sending stratum request to %s (%s) : %s", server.socket.ToStringAddrPort(), server.name, request);
    evbuffer *output = bufferevent_get_output(server.bev);
    if (evbuffer_add(output, request.data(), request.size())) {
        LogPrint(BCLog::MERGEMINE, "Sending stratum request failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
    }

    return id;
}

static void RegisterMergeMineClient(AuxWorkServer& server, const std::string& username, const std::string& password) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    // Send mining.aux.authorize message
    int id = SendAuxAuthorizeRequest(server, username, password);

    // Log the id of the message so that reply (which contains the user's
    // canonical adddress string) can be matched.
    server.aux_auth_jreqid[id] = username;
}

void RegisterMergeMineClient(const ChainId& chainid, const std::string& username, const std::string& password)
{
    LOCK(cs_mergemine);

    // Lookup server from chainid
    AuxWorkServer* server = GetServerFromChainId(chainid, __func__);
    if (!server) {
        throw std::runtime_error(strprintf("No active connection to chainid 0x%s", HexStr(chainid)));
    }

    RegisterMergeMineClient(*server, username, password);
}

std::map<ChainId, AuxWork> GetMergeMineWork(const std::map<ChainId, std::pair<std::string, std::string> >& auth)
{
    LOCK(cs_mergemine);

    // Return value is a mapping of chainid -> AuxWork
    std::map<ChainId, AuxWork> ret;

    // For each chain (identified by chainid), the caller has supplied a
    // username:password pair of authentication credentials.
    for (const auto& item : auth) {
        const ChainId& chainid = item.first;
        const std::string& username = item.second.first;
        const std::string& password = item.second.second;

        // Lookup the server
        AuxWorkServer* server = GetServerFromChainId(chainid, __func__);
        if (!server) {
            LogPrint(BCLog::MERGEMINE, "Requested work for chain 0x%s but no active connection to that chain.\n", HexStr(chainid));
            continue;
        }

        // Lookup the canonical address for the user.
        if (!server->aux_auth.count(username)) {
            LogPrint(BCLog::MERGEMINE, "Requested work for chain 0x%s but user \"%s\" is not registered.\n", HexStr(chainid), username);
            RegisterMergeMineClient(*server, username, password);
            continue;
        }
        const std::string address = server->aux_auth[username];

        // Check to see if there is any work available for this user.
        if (!server->aux_work.count(address)) {
            LogPrint(BCLog::MERGEMINE, "No work available for user \"%s\" (\"%s\") on chain 0x%s\n", username, address, HexStr(chainid));
            continue;
        }
        LogPrint(BCLog::MERGEMINE, "Found work for user \"%s\" (\"%s\") on chain 0x%s", username, address, HexStr(chainid));
        ret[chainid] = server->aux_work[address];
    }

    // Now add work for chains that the caller did provide credentials, but for
    // which default credentials have been provided.
    for (auto& item : g_mergemine_conn) {
        AuxWorkServer& server = item.second;
        // Skip if no default credentials.
        if (!server.default_auth) {
            LogPrint(BCLog::MERGEMINE, "No default credentials for chain 0x%s\n", HexStr(server.aux_pow_path));
            continue;
        }
        // Skip if we already have work for this chain (because the caller
        // provided credentials).
        const ChainId& chainid = server.aux_pow_path;
        if (ret.count(chainid)) {
            LogPrint(BCLog::MERGEMINE, "Already have work for chain 0x%s\n", HexStr(chainid));
            continue;
        }
        // Skip if the default credentials are not registered.
        const std::string& username = server.default_auth->first;
        const std::string& password = server.default_auth->second;
        if (!server.aux_auth.count(username)) {
            // Default credentials are not registered yet.
            LogPrint(BCLog::MERGEMINE, "Default credentials for chain 0x%s are not registered yet.\n", HexStr(chainid));
            RegisterMergeMineClient(server, username, password);
            continue;
        }
        // Skip if there is no work available.
        const std::string& address = server.aux_auth[username];
        if (!server.aux_work.count(address)) {
            LogPrint(BCLog::MERGEMINE, "No work available for default user \"%s\" (\"%s\") on chain 0x%s\n", username, address, HexStr(chainid));
            continue;
        }
        // Add work for this chain.
        LogPrint(BCLog::MERGEMINE, "Adding work for default user \"%s\" (\"%s\") on chain 0x%s\n", username, address, HexStr(chainid));
        ret[chainid] = server.aux_work[address];
    }

    // Return all found work units back to caller.
    return ret;
}

std::optional<std::pair<ChainId, SecondStageWork> > GetSecondStageWork(std::optional<ChainId> hint)
{
    LOCK(cs_mergemine);

    // If the caller was already mining a second stage work unit for a
    // particular chain, be sure to return the current second stage work unit
    // for that chain, to prevent unnecessary work resets.
    if (hint && g_second_stage.count(*hint)) {
        return std::make_pair(*hint, g_second_stage[*hint]);
    }

    // If there is any second-stage work available, return whichever one is
    // easiest to fetch.
    if (!g_second_stage.empty()) {
        auto itr = g_second_stage.begin();
        return std::make_pair(itr->first, itr->second);
    }

    // Report that there is no second-stage work available.
    return std::nullopt;
}

static std::string GetRegisteredAddress(const AuxWorkServer& server, const std::optional<std::string>& username) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    std::string key;
    if (username.has_value()) {
        key = username.value();
    } else {
        if (server.default_auth.has_value()) {
            key = server.default_auth->first; // username
        } else {
            // We got called with a default credential for a chain that has no
            // default credentials.  This should never, ever happen.  We
            // handle it partly to ensure that code analysis tools don't
            // complain, and also to provide proper logging for the obscure,
            // twisted edge cases where this might actually happen (e.g. the
            // auxiliary work server was configured to support default
            // credentials, and then was restared without that support).
            if (!server.aux_auth.empty()) {
                key = server.aux_auth.begin()->first;
            } else {
                key = "";
            }
            LogPrint(BCLog::MERGEMINE, "Submitted work for chain 0x%s with default credentials, but auxiliary work server does not support default credentials; using \"%s\" as username.\n", HexStr(server.aux_pow_path), key);
        }
    }
    std::string ret;
    if (server.aux_auth.count(key)) {
        ret = server.aux_auth.at(key);
    } else {
        // This should never happen.  Nevertheless, we don't want to throw
        // shares away.  Usually the username is the address, so let's assume
        // that and hope for the best.
        ret = key;
        // Remove the "+opts" suffix from the username, if present
        size_t pos = 0;
        if ((pos = ret.find('+')) != std::string::npos) {
            ret.resize(pos);
        }
        boost::trim(ret);
        LogPrint(BCLog::MERGEMINE, "Submitted work for chain 0x%s but user \"%s\" is not registered; assuming address is \"%s\".\n", HexStr(server.aux_pow_path), key, ret);
    }
    return ret;
}

void SubmitAuxChainShare(const ChainId& chainid, const std::optional<std::string>& username, const AuxWork& work, const AuxProof& proof)
{
    LOCK(cs_mergemine);

    // Record the time at which we received this share.
    int64_t now = GetTime();

    // Lookup the server corresponding to this chainid:
    AuxWorkServer* server = GetServerFromChainId(chainid, __func__);
    if (!server) {
        // The following should never happen.  Nevertheless, we don't want to
        // end up in an infinite loop handing out stale work if it does, so we
        // remove any record of the disconnected server.
        if (g_second_stage.count(chainid)) {
            LogPrint(BCLog::MERGEMINE, "Auxiliary work server for chain 0x%s is not connected; discarding share.\n", HexStr(chainid));
            g_second_stage.erase(chainid);
        }
        return;
    }

    // Lookup the registered address for the user.
    const std::string address = GetRegisteredAddress(*server, username);

    // Submit the share to the server:
    SendAuxSubmitRequest(*server, address, work, proof);
    server->last_submit_time = now;
}

void SubmitSecondStageShare(const ChainId& chainid, const std::optional<std::string>& username, const SecondStageWork& work, const SecondStageProof& proof)
{
    LOCK(cs_mergemine);

    // Record the time at which we received this share.
    int64_t now = GetTime();

    // Lookup the server corresponding to this chainid:
    AuxWorkServer* server = GetServerFromChainId(chainid, __func__);
    if (!server) {
        return;
    }

    // Lookup the registered address for the user.
    const std::string address = GetRegisteredAddress(*server, username);

    SendSubmitRequest(*server, address, work, proof);
    server->last_submit_time = now;
}

static UniValue stratum_mining_aux_notify(AuxWorkServer& server, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    // last_recv_time was set to the current time when we started processing
    // this message.  We set last_work_time to the current time so we know
    // when the server last gave us a work unit.
    server.last_work_time = server.last_recv_time;

    const std::string method("mining.aux.notify");
    BoundParams(method, params, 5, 5);

    // Timestamp for the work
    uint64_t time = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());

    // The job_id's internal structure is technically not defined.  For maximum
    // compatibility we make no assumptions and store it in the format in which
    // we received it: a string.
    std::string job_id = params[0].get_str();

    // The second parameter is the list of commitments for each registered miner
    // on this channel.  We'll come back to that.

    // The third parameter is the base difficulty of the share.
    uint32_t bits = ParseHexInt4(params[2], "nBits");

    // The fourth value is the share's bias setting.
    int bias = params[3].getInt<int>();
    if (bias < 0 || 255 < bias) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "aux proof-of-work bias value out of range");
    }

    // If there is currently a second-stage work unit for this chain, then clear
    // it out.  Receipt of new mining.aux.notify message indicates that a block
    // has been solved.
    if (g_second_stage.count(server.aux_pow_path)) {
        LogPrint(BCLog::MERGEMINE, "Removing unsolved second-stage work unit for chain 0x%s", HexStr(server.aux_pow_path));
        g_second_stage.erase(server.aux_pow_path);
    }

    std::map<std::string, uint256> work;
    const UniValue& commits = params[1].get_obj();
    for (const std::string& address : commits.getKeys()) {
        // The commitment is a 256-bit hash.
        uint256 commit = ParseUInt256(commits[address].get_str(), "commit");
        LogPrint(BCLog::MERGEMINE, "Got commitment for aux address \"%s\" on chain 0x%s: 0x%s %s,%d\n", address, HexStr(server.aux_pow_path), commit.ToString(), HexInt4(bits), bias);
        server.aux_work[address] = AuxWork(time, job_id, commit, bits, bias);
    }

    // Trigger stratum work reset.
    g_best_block_cv.notify_all();

    return true;
}

static UniValue stratum_mining_set_difficulty(AuxWorkServer& server, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    const std::string method("mining.set_difficulty");
    BoundParams(method, params, 1, 1);

    server.diff = params[0].get_real();

    return true;
}

static UniValue stratum_mining_notify(AuxWorkServer& server, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    // last_recv_time was set to the current time when we started processing
    // this message.  We set last_work_time to the current time so we know
    // when the server last gave us a work unit.
    server.last_work_time = server.last_recv_time;

    const std::string method("mining.notify");
    BoundParams(method, params, 8, 9);

    // Timestamp for the work
    uint64_t time = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());

    // As above, we ignore whatever internal structure there might be to job_id.
    // It's just a standard string to us.
    std::string job_id = params[0].get_str();

    // Stratum byte-swaps the hashPrevBlock for unknown reasons.
    uint256 hashPrevBlock = ParseUInt256(params[1].get_str(), "hashPrevBlock");
    for (int i = 0; i < 256/32; ++i) {
        ((uint32_t*)hashPrevBlock.begin())[i] = bswap_32(
        ((uint32_t*)hashPrevBlock.begin())[i]);
    }

    // The next two fields are the split coinbase transaction.
    std::vector<unsigned char> cb1 = ParseHexV(params[2], "cb1");
    std::vector<unsigned char> cb2 = ParseHexV(params[3], "cb2");

    std::vector<uint256> cb_branch;
    UniValue branch = params[4].get_array();
    for (int i = 0; i < branch.size(); ++i) {
        cb_branch.push_back(ParseUInt256(branch[i].get_str(), strprintf("cb_branch[%d]", i)));
    }

    uint32_t nVersion = ParseHexInt4(params[5], "nVersion");
    uint32_t nBits = ParseHexInt4(params[6], "nBits");
    uint32_t nTime = ParseHexInt4(params[7], "nTime");

    bool reset = false;
    if (g_second_stage.count(server.aux_pow_path) && (g_second_stage[server.aux_pow_path].hashPrevBlock != hashPrevBlock)) {
        reset = true;
    }
    if ((params.size() > 8) && params[8].get_bool()) {
        reset = true;
    }

    if (g_second_stage.count(server.aux_pow_path)) {
        LogPrint(BCLog::MERGEMINE, "Replacing second stage work unit for chain 0x%s\n", HexStr(server.aux_pow_path));
    }
    g_second_stage[server.aux_pow_path] = SecondStageWork(time, server.diff, job_id, hashPrevBlock, cb1, cb2, cb_branch, nVersion, nBits, nTime);

    // Trigger stratum work reset.
    if (reset) {
        g_best_block_cv.notify_all();
    }

    return true;
}

static void merge_mining_read_cb(bufferevent *bev, void *ctx)
{
    LOCK(cs_mergemine);
    // Lookup the client record for this connection
    if (!g_mergemine_conn.count(bev)) {
        LogPrint(BCLog::MERGEMINE, "Received read notification for unknown auxiliary work source on connection 0x%x\n", (size_t)bev);
        return;
    }
    AuxWorkServer& server = g_mergemine_conn[bev];
    // Get links to the input and output buffers
    evbuffer *input = bufferevent_get_input(bev);
    evbuffer *output = bufferevent_get_output(bev);
    // Process each line of input that we have received
    bool done = false;
    char *cstr = 0;
    size_t len = 0;
    while (!done && (cstr = evbuffer_readln(input, &len, EVBUFFER_EOL_CRLF))) {
        // Update the last message received timestamp for the server.
        server.last_recv_time = GetTime();

        // Convert the line to a std::string with managed memory.
        std::string line(cstr, len);
        free(cstr);

        // Log the communication.
        LogPrint(BCLog::MERGEMINE, "Received line of data from auxiliary work source stratum+tcp://%s (%s) : %s\n", server.socket.ToStringAddrPort(), server.name, line);

        // Parse response
        bool isreply = false;
        UniValue val;
        try {
            if (!val.read(line)) {
                // Not JSON; is this even a stratum server?
                throw std::runtime_error("JSON parse error; skipping line");
            }
            if (!val.isObject()) {
                // Not a JSON object; don't know what to do.
                throw std::runtime_error("Top-level object parse error; skipping line");
            }
            if (val.exists("result") && val.exists("id")) {
                isreply = true;
                const int id = val["id"].getInt<int>();
                if ((id == -1 || id == -2) && (server.idflags & id)) {
                    server.idflags ^= -id;
                } else if (!server.aux_auth_jreqid.count(id)) {
                    // Not a JSON-RPC reply we care about.  Ignore.
                    throw std::runtime_error("Ignorable stratum response");
                }
                // Is JSON reply we're expecting.
                // Will handle later.
            }
            else if (!val.exists("method") || !val.exists("params")) {
                throw std::runtime_error("Ignoring JSON message that is not a stratum request/response");
            }
        } catch (const std::exception& e) {
            // Whatever we received wasn't the response we were looking for.
            // Ignore.
            LogPrint(BCLog::MERGEMINE, "Received line of data from auxiliary work source stratum+tcp://%s (%s): %s\n", server.socket.ToStringAddrPort(), server.name, e.what());
            continue;
        }

        try {
            if (isreply && val["id"].getInt<int>() == -1) {
                // val contains the reply to our mining.subscribe request, from
                // which we extract the extranonce information.  Any errors from
                // here on are fatal.
                if (val.exists("error") && !val["error"].isNull()) {
                    // mining.subscribe was not understood.
                    throw std::runtime_error("does not support mining.subscribe");
                }
                const UniValue& result = val["result"];
                if (!result.isArray() || result.empty() || !result[1].isStr() || !result[2].isNum()) {
                    // expected fields of mining.subscribe response are not present.
                    throw std::runtime_error("mining.subscribe response was ill-formed");
                }
                std::vector<unsigned char> extranonce1;
                try {
                    extranonce1 = ParseHexV(result[1], "extranonce1");
                } catch (...) {
                    // result[1] is not a hex string
                    throw std::runtime_error("expected hex-encoded extranonce1 as second value of minign.subscribe response");
                }
                int extranonce2_size = 0;
                try {
                    extranonce2_size = result[2].getInt<int>();
                } catch (...) {
                    // result[2] could not be cast to integer
                    throw std::runtime_error("expected integer extranonce2_size as third value of mining.subscribe response");
                }
                LogPrint(BCLog::MERGEMINE, "Setting extranonce1 to \"%s\" and extranonce2_size to %d from stratum+tcp://%s (%s)\n", HexStr(extranonce1), extranonce2_size, server.socket.ToStringAddrPort(), server.name);
                swap(server.extranonce1, extranonce1);
                server.extranonce2_size = extranonce2_size;
            }

            if (isreply && val["id"].getInt<int>() == -2) {
                // val contains the reply to our mining.aux.subscribe request.
                // Any errors from here on are fatal.
                if (val.exists("error") && !val["error"].isNull()) {
                    // mining.aux.subscribe was not understood.
                    throw std::runtime_error("does not support mining.aux.subscribe");
                }
                const UniValue& result = val["result"];
                if (!result.isArray() || result.empty() || !result[0].isStr()) {
                    // expected fields of mining.aux.subscribe response are not present.
                    throw std::runtime_error("response was ill-formed");
                }
                ChainId aux_pow_path;
                try {
                    aux_pow_path = ChainId(ParseUInt256(result[0], "aux_pow_path"));
                } catch (...) {
                    // result[0] was not a uint256
                    throw std::runtime_error("expected hex-encoded aux_pow_path as first value of mining.aux.subscribe response");
                }
                server.aux_pow_path = aux_pow_path;
                if (g_mergemine.count(aux_pow_path)) {
                    // Already have a source for this chain
                    throw std::runtime_error("already have auxiliary work source for this chain");
                }
                // result[1] is MAX_AUX_POW_COMMIT_BRANCH_LENGTH
                //
                // This is the per-chain limit on the maximum number of skip
                // hashes that can be included in a commit proof.  At this
                // time we don't actually respect this limit on our end, as
                // there are not enough merge-mined chains for this to
                // actually be an issue.
                //
                // result[2] is MAX_AUX_POW_BRANCH_LENGTH
                //
                // This is the per-chain limit on the maximum number of hashes
                // that can be included in the Merkle proof of the bitoin
                // block-final transaction, in which the merge-mining
                // commitment is stored.  We don't bother with this limit
                // because at this time all known merge-mining chains have
                // limits set high enough that this can never be an issue
                // unless bitcoin's blocksize is significantly increased.
                if (default_auths.count(aux_pow_path)) {
                    // We have default credentials for this chain, overriding
                    // whatever defaults were sent by the server.
                    server.default_auth = default_auths[aux_pow_path];
                } else if (result.size() >= 4 && result[3].isArray() && result[3].size() >= 2 && result[3][0].isStr() && result[3][1].isStr()) {
                    // result[3] is the default connection credentials, to be
                    // used if no credentials are provided for this chain by
                    // the miner.
                    server.default_auth = std::make_pair(result[3][0].get_str(), result[3][1].get_str());
                }
                // Report the auxiliary work server as online in the logs.
                LogPrintf("Registering auxiliary work notifications for chain 0x%s from stratum+tcp://%s (%s)%s\n", HexStr(aux_pow_path), server.socket.ToStringAddrPort(), server.name, server.default_auth ? " with default credentials" : "");
                g_mergemine[aux_pow_path] = bev;
                // If we have default credentials, open a subscription channel
                // using those credentials to receive work update notifications.
                if (server.default_auth) {
                    RegisterMergeMineClient(server, server.default_auth->first, server.default_auth->second);
                }
            }
        } catch (const std::exception& e) {
            LogPrint(BCLog::MERGEMINE, "Received %s response from stratum+tcp://%s (%s): %s\n", val["id"].getInt<int>()==-1 ? "mining.subscribe" : "mining.aux.subscribe", server.socket.ToStringAddrPort(), server.name, e.what());
            LogPrintf("Unable to subscribe to auxiliary work notifications from stratum+tcp://%s (%s) ; not adding\n", server.socket.ToStringAddrPort(), server.name);
            // Do nothing here.  The connection will be torn down without the
            // server having been added as a source of auxiliary work units.
            done = true;
        }
        if (isreply) {
            try {
                int id = val["id"].getInt<int>();
                if (server.aux_auth_jreqid.count(id)) {
                    std::string username(server.aux_auth_jreqid[id]);
                    std::string address(val["result"].get_str());
                    LogPrint(BCLog::MERGEMINE, "Mapping username \"%s\" to remote address \"%s\" for stratum+tcp://%s (%s)\n", username, address, server.socket.ToStringAddrPort(), server.name);
                    server.aux_auth[username] = address;
                    server.aux_auth_jreqid.erase(id);
                }
            } catch (const std::exception& e) {
                LogPrint(BCLog::MERGEMINE, "Error while handling response from stratum+tcp://%s (%s): %s\n", server.socket.ToStringAddrPort(), server.name, e.what());
            }
            continue;
        }

        JSONRPCRequest jreq;
        std::string reply;
        try {
            jreq.parse(val);

            // Dispatch to method handler
            UniValue result = NullUniValue;
            if (stratum_method_dispatch.count(jreq.strMethod)) {
                result = stratum_method_dispatch[jreq.strMethod](server, jreq.params);
            } else {
                throw JSONRPCError(RPC_METHOD_NOT_FOUND, strprintf("Method '%s' not found", jreq.strMethod));
            }

            // Compose reply
            reply = JSONRPCReply(result, NullUniValue, jreq.id);
        } catch (const UniValue& objError) {
            // If a JSON object is thrown, we can encode it directly.
            reply = JSONRPCReply(NullUniValue, objError, jreq.id);
        } catch (const std::exception& e) {
            // Otherwise turn the error into a string and return that.
            reply = JSONRPCReply(NullUniValue, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
        }

        // Send reply
        LogPrint(BCLog::STRATUM, "Sending stratum response to stratum+tcp://%s (%s) : %s", server.socket.ToStringAddrPort(), server.name, reply);
        if (evbuffer_add(output, reply.data(), reply.size())) {
            LogPrint(BCLog::STRATUM, "Sending stratum response failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
        }
    }

    if (done) {
        LogPrint(BCLog::MERGEMINE, "Closing initial stratum connection to stratum+tcp://%s (%s)\n", server.socket.ToStringAddrPort(), server.name);
        // Only ever reach here if the server wasn't fully setup, so it won't be
        // in the mergemine chain id -> connection map.
        g_mergemine_conn.erase(bev);
        if (bev) {
            bufferevent_free(bev);
            bev = nullptr;
        }
    }

    // Attempt to re-establish any dropped connections.
    LocalReconnectToMergeMineEndpoints();
}

static void merge_mining_event_cb(bufferevent *bev, short what, void *ctx)
{
    LOCK(cs_mergemine);
    // Lookup the client record for this connection
    if (!g_mergemine_conn.count(bev)) {
        LogPrint(BCLog::MERGEMINE, "Received event notification for unknown auxiliary work source on connection 0x%x\n", (size_t)bev);
        return;
    }
    AuxWorkServer& server = g_mergemine_conn[bev];
    // Report the reason why we are closing the connection.
    if (what & BEV_EVENT_ERROR) {
        LogPrint(BCLog::MERGEMINE, "Error detected on initial stratum connection to stratum+tcp://%s (%s)\n", server.socket.ToStringAddrPort(), server.name);
    }
    if (what & BEV_EVENT_EOF) {
        LogPrint(BCLog::MERGEMINE, "Remote disconnect received on stratum connection to stratum+tcp://%s (%s)\n", server.socket.ToStringAddrPort(), server.name);
    }
    // Remove the connection from our records, and tell libevent to disconnect
    // and free its resources.
    if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        // Remove any second-stage work unit associated with the server.
        if (g_second_stage.count(server.aux_pow_path)) {
            LogPrint(BCLog::MERGEMINE, "Removing second-stage work unit for chain 0x%s from stratum+tcp://%s (%s) (reason: disconnect)\n", HexStr(server.aux_pow_path), server.socket.ToStringAddrPort(), server.name);
            g_second_stage.erase(server.aux_pow_path);
        }
        // Remove connection from g_mergemine.
        if (g_mergemine.count(server.aux_pow_path) && g_mergemine[server.aux_pow_path] == bev) {
            LogPrintf("Unregistering auxiliary work notifications for chain 0x%s from stratum+tcp://%s (%s)\n", HexStr(server.aux_pow_path), server.socket.ToStringAddrPort(), server.name);
            g_mergemine.erase(server.aux_pow_path);
        }
        // Add connection to g_mergemine_noconn.
        LogPrintf("Queing stratum+tcp://%s (%s) for later reconnect\n", server.socket.ToStringAddrPort(), server.name);
        g_mergemine_noconn[server.socket] = AuxServerDisconnect(server.name);
        // Remove connection from g_mergemine_conn.
        LogPrint(BCLog::MERGEMINE, "Closing initial stratum connection to stratum+tcp://%s (%s)\n", server.socket.ToStringAddrPort(), server.name);
        g_mergemine_conn.erase(bev);
        if (bev) {
            bufferevent_free(bev);
            bev = nullptr;
        }
    }

    // Attempt to re-establish any dropped connections.
    LocalReconnectToMergeMineEndpoints();
}

static bufferevent* ConnectToAuxWorkServer(const std::string& name, const CService& socket)
{
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        LogPrintf("Unable to create bufferevent object for merge-mining initialization\n");
        return nullptr;
    }

    bufferevent_setcb(bev, merge_mining_read_cb, nullptr, merge_mining_event_cb, nullptr);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    sockaddr addr;
    socklen_t len = sizeof(addr);
    socket.GetSockAddr(&addr, &len);
    if (bufferevent_socket_connect(bev, &addr, len) < 0) {
        LogPrintf("Unable to connect to stratum+tcp://%s (%s)\n", socket.ToStringAddrPort(), name);
        bufferevent_free(bev);
        bev = nullptr;
        return nullptr;
    }

    return bev;
}

static void SendSubscribeRequest(AuxWorkServer& server) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    LogPrintf("Sending request to source merge-mine work from stratum+tcp://%s (%s)\n", server.socket.ToStringAddrPort(), server.name);

    static const std::string request =
        "{\"id\":-1,\"method\":\"mining.subscribe\",\"params\":[\""
        + FormatFullVersion()
        + "\"]}\n{\"id\":-2,\"method\":\"mining.aux.subscribe\",\"params\":[]}\n";

    LogPrint(BCLog::MERGEMINE, "Sending stratum request to %s (%s) : %s", server.socket.ToStringAddrPort(), server.name, request);
    evbuffer *output = bufferevent_get_output(server.bev);
    if (evbuffer_add(output, request.data(), request.size())) {
        LogPrint(BCLog::MERGEMINE, "Sending stratum request failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
    } else {
        server.idflags = 3;
    }
}

static bufferevent* ConnectToStratumEndpoint(const CService& socket, const AuxServerDisconnect& conn) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    bufferevent *bev;

    // Attempt a connection to the stratum endpoint.
    if ((bev = ConnectToAuxWorkServer(conn.name, socket))) {
        // Record the connection as active.
        g_mergemine_conn[bev] = AuxWorkServer(conn.name, socket, bev);
        // Send the stratum subscribe and aux.subscribe messages.
        SendSubscribeRequest(g_mergemine_conn[bev]);
    } else {
        // Unable to connect at this time; will retry later.
        LogPrintf("Queing stratum+tcp://%s (%s) for later reconnect\n", socket.ToStringAddrPort(), conn.name);
        g_mergemine_noconn[socket] = AuxServerDisconnect(conn.name);
    }

    return bev;
}

// To avoid requiring re-entrant mutexes, we have two versions of this function,
// one for calling within this file where LOCK(cs_mergemine) is already held,
// and an externally visible version which first aquires the lock.
void ReconnectToMergeMineEndpoints()
{
    LOCK(cs_mergemine);
    LocalReconnectToMergeMineEndpoints();
}

static void LocalReconnectToMergeMineEndpoints() EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    uint64_t now = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());

    std::map<CService, AuxServerDisconnect> endpoints;
    for (const auto& endpoint : g_mergemine_noconn) {
        const CService& socket = endpoint.first;
        const AuxServerDisconnect& conn = endpoint.second;
        if (now >= (conn.timestamp + (15*1000))) {
            endpoints[socket] = conn;
        }
    }

    for (const auto& endpoint : endpoints) {
        const CService& socket = endpoint.first;
        const AuxServerDisconnect& conn = endpoint.second;
        LogPrintf("Attempting reconnect to stratum+tcp://%s (%s)\n", socket.ToStringAddrPort(), conn.name);
        g_mergemine_noconn.erase(socket);
        bufferevent* bev = ConnectToStratumEndpoint(socket, conn);
    }
}

static int DefaultMergeMinePort()
{
    const ChainType& type = gArgs.GetChainType();
    if (type == ChainType::REGTEST) {
        return 29638;
    }
    if (type == ChainType::TESTNET) {
        return 19638;
    }
    return 9638;
}

static std::atomic<bool> g_shutdown = false;
void MergeMiningManagerThread()
{
    int defaultPort = DefaultMergeMinePort();

    if (!gArgs.IsArgSet("-mergeminename")) {
        chain_names.clear();
        // Freicoin / Tradecraft main network.
        chain_names["freicoin"] = CreateChainParams(gArgs, ChainType::MAIN)->DefaultAuxPowPath();
        chain_names["tradecraft"] = chain_names["freicoin"];
        // Freicoin / Tradecraft test network.
        chain_names["testnet"] = CreateChainParams(gArgs, ChainType::TESTNET)->DefaultAuxPowPath();
        // Freicoin / Tradecraft RPC test network.
        chain_names["regtest"] = CreateChainParams(gArgs, ChainType::REGTEST)->DefaultAuxPowPath();
    } else {
        // Setup the text string -> aux_pow_path map from conf-file settings:
        const std::vector<std::string>& vnames = gArgs.GetArgs("-mergeminename");
        for (std::vector<std::string>::const_iterator i = vnames.begin(); i != vnames.end(); ++i) {
            size_t pos = i->find(':');
            if (pos == std::string::npos) {
                LogPrintf("Unable to parse -mergeminename=\"%s\". Needs to be in the format \"name:chainid\".\n", *i);
                continue;
            }
            std::string name(*i, 0, pos);
            std::string chainid_str(*i, pos+1);
            ChainId chainid;
            try {
                chainid = ChainId(ParseUInt256(chainid_str, "chainid"));
            } catch (...) {
                LogPrintf("Unable to convert \"%s\" to uint256. Not a proper chain id?\n", chainid_str);
                continue;
            }
            LogPrintf("Adding \"%s\" as alternative name for merge-mine chain id 0x%s\n", name, HexStr(chainid));
            chain_names[name] = chainid;
        }
    }

    if (gArgs.IsArgSet("-mergeminedefault")) {
        // Setup the chainid -> (username, password) defaults from conf-file settings:
        const std::vector<std::string>& vdefaults = gArgs.GetArgs("-mergeminedefault");
        for (const auto& conf : vdefaults) {
            size_t pos = conf.find(':');
            if (pos == std::string::npos) {
                LogPrintf("Unable to parse -mergeminedefault=\"%s\". Needs to be in the format \"name:username[:password]\" or \"chainid:username[:password]\".\n", conf);
                continue;
            }
            std::string chainid_str(conf, 0, pos);
            std::string userpass(conf, pos+1);
            ChainId chainid;
            if (chain_names.count(chainid_str)) {
                chainid = chain_names[chainid_str];
            } else {
                try {
                    chainid = ChainId(ParseUInt256(chainid_str, "chainid"));
                } catch (...) {
                    LogPrintf("Unable to convert \"%s\" to uint256. Not a proper chain id?\n", chainid_str);
                    continue;
                }
            }
            size_t pos2 = userpass.find(':');
            std::string username(userpass);
            std::string password;
            if (pos2 != std::string::npos) {
                password = std::string(userpass, pos2+1);
                username.resize(pos2);
            }
            LogPrintf("Adding default username \"%s\"%s for merge-mine chain id 0x%s%s\n", username, password.empty() ? "" : " with password", HexStr(chainid), default_auths.count(chainid) ? " (overriding previous setting)" : "");
            default_auths[chainid] = std::make_pair(username, password);
        }
    }

    if (gArgs.IsArgSet("-mergemine")) {
        // Servers which we will attempt to to establish a connection to.
        std::map<CService, std::string> servers;

        const std::vector<std::string>& vmerge = gArgs.GetArgs("-mergemine");
        for (std::vector<std::string>::const_iterator i = vmerge.begin(); i != vmerge.end(); ++i) {
            std::optional<CService> socket = Lookup(i->c_str(), defaultPort, true);
            if (!socket.has_value()) {
                LogPrintf("Invalid socket address for -mergemine endpoint: %s ; skipping\n", *i);
                continue;
            }

            if (servers.count(*socket)) {
                LogPrintf("Duplicate -mergemine endpoint: %s (same as %s) ; skipping\n", *i, servers[*socket]);
                continue;
            }

            servers[*socket] = *i;
        }

        for (std::map<CService, std::string>::const_iterator i = servers.begin(); i != servers.end(); ++i) {
            const CService& socket = i->first;
            const std::string& name = i->second;
            ConnectToStratumEndpoint(socket, name);
            // Handle any events that have been triggered by our actions so far.
            event_base_loop(base, EVLOOP_NONBLOCK);
        }
    }

    // Shutdown the thread if there are no connections to manage.
    if (g_mergemine_conn.empty() && g_mergemine_noconn.empty()) {
        LogPrint(BCLog::MERGEMINE, "No merge-mining connections to manage, exiting event dispatch thread.\n");
        return;
    }

    // Ready to start processing events.
    // Set the start time to the current time.
    start_time = GetTime();

    // Set the timeout for the event dispatch loop.
    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    LogPrint(BCLog::MERGEMINE, "Entering merge-mining event dispatch loop\n");
    while (!g_shutdown) {
        // Attempt to re-establish any connections that have been dropped.
        ReconnectToMergeMineEndpoints();

        // Enter event dispatch loop, with a 15 second timeout.
        event_base_loopexit(base, &timeout);
        event_base_dispatch(base);
    }
    LogPrint(BCLog::MERGEMINE, "Exited merge-mining event dispatch loop\n");

    // Clear the start_time to indicate that the event dispatch loop is no
    // longer runing.
    start_time = 0;
}

/** Configure the merge-mining subsystem. */
bool InitMergeMining(NodeContext& node)
{
    // Assertion is allowed because this is initialization code.  If base is
    // non-NULL, then we have been called twice, and that is a serious bug.
    assert(!base);

#ifdef WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif
    base = event_base_new();
    if (!base) {
        LogPrintf("Unable to create event_base object, cannot setup merge-mining.\n");
        return false;
    }

    g_context = &node;

    stratum_method_dispatch["mining.aux.notify"] = stratum_mining_aux_notify;
    stratum_method_dispatch["mining.set_difficulty"] = stratum_mining_set_difficulty;
    stratum_method_dispatch["mining.notify"] = stratum_mining_notify;

    merge_mining_manager_thread = std::thread(&util::TraceThread, "mergemine", [&] { MergeMiningManagerThread(); });

    return true;
}

/** Interrupt any active network connections. */
void InterruptMergeMining()
{
    // Tell the merge-mining connection manager thread to shutdown.
    g_shutdown = true;
    event_base_loopbreak(base);
}

/** Cleanup network connections made by the merge-mining subsystem, free
 ** associated resources, and cleanup global state. */
void StopMergeMining()
{
    g_shutdown = true;
    event_base_loopbreak(base);
    if (merge_mining_manager_thread.joinable()) {
        merge_mining_manager_thread.join();
    }
    LOCK(cs_mergemine);
    /* Tear-down active connections. */
    for (const auto& bind : g_mergemine) {
        const AuxWorkServer& server = g_mergemine_conn[bind.second];
        LogPrintf("Unregistering auxiliary work notifications for chain 0x%s from stratum+tcp://%s (%s)\n", HexStr(server.aux_pow_path), server.socket.ToStringAddrPort(), server.name);
    }
    g_mergemine.clear();
    for (auto& server : g_mergemine_conn) {
        LogPrint(BCLog::MERGEMINE, "Closing stratum connection to stratum+tcp://%s (%s) due to process termination\n", server.second.socket.ToStringAddrPort(), server.second.name);
        if (server.second.bev) {
            bufferevent_free(server.second.bev);
            server.second.bev = nullptr;
        }
    }
    g_mergemine_conn.clear();
    g_mergemine_noconn.clear();
    /* Destroy the libevent context. */
    if (base) {
        event_base_free(base);
        base = nullptr;
    }
}

static RPCHelpMan getmergemineinfo() {
    return RPCHelpMan{"getmergemineinfo",
        "\nReturns an object containing various state info regarding the merge-mining coordination server.\n",
        {},
        RPCResult{RPCResult::Type::OBJ, "", "", {
            {RPCResult::Type::BOOL, "enabled", "whether the merged-mining coordination server is running or not"},
            {RPCResult::Type::NUM_TIME, "curtime", /*optional=*/true, "the current time, in seconds since UNIX epoch"},
            {RPCResult::Type::NUM_TIME, "uptime", /*optional=*/true, "the merged-mining coordination server uptime, in seconds"},
            {RPCResult::Type::OBJ_DYN, "connected", /*optional=*/true, "A map of chain IDs to descriptive objects for each connected merge-mined subchain", {
                {RPCResult::Type::OBJ, "name", "The connection string for the server, as specified in the config file", {
                    {RPCResult::Type::STR, "chainid", "the chain ID"},
                    {RPCResult::Type::STR, "netaddr", "The network address used to connect to the server, resolved from th connection string"},
                    {RPCResult::Type::NUM_TIME, "conntime", "the time elapsed since the connection was established, in seconds"},
                    {RPCResult::Type::NUM_TIME, "lastrecv", /*optional=*/true, "the time elapsed since the last message was received, in seconds"},
                    {RPCResult::Type::NUM_TIME, "lastjob", /*optional=*/true, "the time elapsed since the last updated work unit was received from the server, in seconds"},
                    {RPCResult::Type::NUM_TIME, "lastshare", /*optional=*/true, "the time elapsed since the last share we submitted to the server, in seconds"},
                    {RPCResult::Type::NUM, "difficulty", "the current submission difficulty for subchain shares"},
                    {RPCResult::Type::OBJ, "defaultauth", "The default authentication credentials for the server", {
                        {RPCResult::Type::STR, "username", "the username"},
                        {RPCResult::Type::STR, "password", "the password"},
                    }},
                    {RPCResult::Type::OBJ_DYN, "auxauth", "A map of usernames to remote addresses for each connected user", {
                        {RPCResult::Type::STR, "username", "the remote address of the user"},
                    }},
                }},
            }},
            {RPCResult::Type::OBJ_DYN, "disconnects", /*optional=*/true, "An array of subchain servers awaiting reconnection attempts", {
                {RPCResult::Type::OBJ, "name", "The connection string for the server, as specified in the config file", {
                    {RPCResult::Type::STR, "netaddr", "The network address used to connect to the server, resolved from th connection string"},
                    {RPCResult::Type::NUM_TIME, "since", "How long ago in seconds that communication with the server was lost"},
                }},
            }},
            {RPCResult::Type::OBJ_DYN, "secondstage", "A map of chain IDs to second stage work units that are currently being worked on", {
                {RPCResult::Type::OBJ, "chainid", "the chain ID", {
                    {RPCResult::Type::STR_HEX, "jobid", "the job ID"},
                    {RPCResult::Type::STR_HEX, "target", "the difficulty converted into a target hash"},
                    {RPCResult::Type::NUM, "difficulty", "the required difficulty of the second stage work unit"},
                    {RPCResult::Type::NUM_TIME, "age", "how long the second stage work unit has been queued, in seconds"},
                }},
            }},
        }},
        RPCExamples{
            HelpExampleCli("getmergemineinfo", "")
            + HelpExampleRpc("getmergemineinfo", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue ret(UniValue::VOBJ);
    bool enabled = (start_time != 0);
    ret.pushKV("enabled", enabled);
    if (!enabled) {
        return ret;
    }
    LOCK(cs_mergemine);
    int64_t now = GetTime();
    ret.pushKV("curtime", now);
    ret.pushKV("uptime", now - start_time);
    UniValue connected(UniValue::VOBJ);
    for (const auto& item : g_mergemine_conn) {
        const AuxWorkServer& server = item.second;
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("chainid", HexStr(server.aux_pow_path));
        obj.pushKV("netaddr", server.socket.ToStringAddrPort());
        obj.pushKV("conntime", now - server.connect_time);
        if (server.last_recv_time > 0) {
            obj.pushKV("lastrecv", now - server.last_recv_time);
        }
        if (server.last_work_time > 0) {
            obj.pushKV("lastjob", now - server.last_work_time);
        }
        if (server.last_submit_time > 0) {
            obj.pushKV("lastshare", now - server.last_submit_time);
        }
        obj.pushKV("difficulty", server.diff);
        if (server.default_auth.has_value()) {
            UniValue defaultauth(UniValue::VOBJ);
            defaultauth.pushKV("username", server.default_auth->first);
            defaultauth.pushKV("password", server.default_auth->second);
            obj.pushKV("defaultauth", defaultauth);
        }
        UniValue auxauth(UniValue::VOBJ);
        for (const auto& item : server.aux_auth) {
            const std::string& name = item.first;
            const std::string& addr = item.second;
            auxauth.pushKV(name, addr);
        }
        obj.pushKV("auxauth", auxauth);
        connected.pushKV(server.name, obj);
    }
    ret.pushKV("connected", connected);
    UniValue disconnects(UniValue::VOBJ);
    for (const auto& item : g_mergemine_noconn) {
        const CService& socket = item.first;
        const AuxServerDisconnect& disconnect = item.second;
        UniValue chain(UniValue::VOBJ);
        chain.pushKV("netaddr", socket.ToStringAddrPort());
        chain.pushKV("since", now - (disconnect.timestamp / 1000));
        disconnects.pushKV(disconnect.name, chain);
    }
    ret.pushKV("disconnects", disconnects);
    UniValue secondstage(UniValue::VOBJ);
    for (const auto& item : g_second_stage) {
        const ChainId& chainid = item.first;
        const SecondStageWork& work = item.second;
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("jobid", HexStr(work.job_id));
        arith_uint256 target;
        target.SetCompact(work.nBits);
        obj.pushKV("target", target.ToString());
        obj.pushKV("difficulty", work.diff);
        obj.pushKV("age", now - (work.timestamp / 1000));
        secondstage.pushKV(HexStr(chainid), obj);
    }
    ret.pushKV("secondstage", secondstage);
    return ret;
},
    };
}

void RegisterMergeMineRPCCommands(CRPCTable& t) {
    static const CRPCCommand commands[]{
        {"mining", &getmergemineinfo},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

// End of File
