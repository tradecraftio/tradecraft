// Copyright (c) 2020-2022 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <mergemine.h>

#include <chainparams.h>
#include <clientversion.h>
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
#include <util/system.h>
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

//! Mapping of alternative names to chain specifiers
std::map<std::string, uint256> chain_names;

//! Merge-mining manager thread
static std::thread merge_mining_manager_thread;

struct AuxWorkServer
{
    std::string name;
    CService socket;
    bufferevent* bev;
    int idflags;
    int nextid;
    std::vector<unsigned char> extranonce1;
    int extranonce2_size;
    uint256 aux_pow_path;
    std::map<int, std::string> aux_auth_jreqid;
    std::map<std::string, std::string> aux_auth;
    std::map<std::string, AuxWork> aux_work;
    double diff;

    AuxWorkServer() : bev(nullptr), idflags(0), nextid(0), extranonce2_size(0), diff(0.0) { }
    AuxWorkServer(const std::string& nameIn, const CService& socketIn, bufferevent* bevIn) : name(nameIn), socket(socketIn), bev(bevIn), idflags(0), nextid(0), extranonce2_size(0), diff(0.0) { }
};

struct AuxServerDisconnect
{
    uint64_t timestamp;
    std::string name;

    AuxServerDisconnect() { timestamp = GetTimeMillis(); }
    AuxServerDisconnect(const std::string& nameIn) : name(nameIn) { timestamp = GetTimeMillis(); }
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
static std::map<uint256, bufferevent*> g_mergemine;

// Forward declaration of stratum endpoint reconnection utility function.
static void LocalReconnectToMergeMineEndpoints();

//! A collection of second-stage work units to be solved, mapping chainid -> SecondStageWork.
static std::map<uint256, SecondStageWork> g_second_stage;

static AuxWorkServer* GetServerFromChainId(const uint256& chainid, const std::string& caller) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
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
    LogPrintf("Authoriing aux-pow work on chain 0x%s through stratum+tcp://%s (%s) for client %s\n", HexStr(server.aux_pow_path), server.socket.ToString(), server.name, username);

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
    LogPrint(BCLog::MERGEMINE, "Sending stratum request to %s (%s) : %s", server.socket.ToString(), server.name, request);
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
    params.push_back(UniValue(UniValue::VARR)); // FIXME: no commit branch
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
    LogPrint(BCLog::MERGEMINE, "Sending stratum request to %s (%s) : %s", server.socket.ToString(), server.name, request);
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
    LogPrint(BCLog::MERGEMINE, "Sending stratum request to %s (%s) : %s", server.socket.ToString(), server.name, request);
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

void RegisterMergeMineClient(const uint256& chainid, const std::string& username, const std::string& password)
{
    LOCK(cs_mergemine);

    // Lookup server from chainid
    AuxWorkServer* server = GetServerFromChainId(chainid, __func__);
    if (!server) {
        throw std::runtime_error(strprintf("No active connection to chainid 0x%s", HexStr(chainid)));
    }

    RegisterMergeMineClient(*server, username, password);
}

std::map<uint256, AuxWork> GetMergeMineWork(const std::map<uint256, std::pair<std::string, std::string> >& auth)
{
    LOCK(cs_mergemine);

    // Return value is a mapping of chainid -> AuxWork
    std::map<uint256, AuxWork> ret;

    // For each chain (identified by chainid), the caller has supplied a
    // username:password pair of authentication credentials.
    for (const auto& item : auth) {
        const uint256& chainid = item.first;
        const std::string& username = item.second.first;
        const std::string& password = item.second.second;

        // Lookup the server
        AuxWorkServer* server = GetServerFromChainId(chainid, __func__);
        if (!server) {
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
        ret[chainid] = server->aux_work[address];
    }

    // Return all found work units back to caller.
    return ret;
}

std::optional<std::pair<uint256, SecondStageWork> > GetSecondStageWork(std::optional<uint256> hint)
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

static std::string GetRegisteredAddress(const AuxWorkServer& server, const std::string& username) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    std::string ret;
    if (server.aux_auth.count(username)) {
        ret = server.aux_auth.at(username);
    } else {
        // This should never happen.  Nevertheless, we don't want to throw
        // shares away.  Usually the username is the address, so let's assume
        // that and hope for the best.
        ret = username;
        // Remove the "+opts" suffix from the username, if present
        size_t pos = 0;
        if ((pos = ret.find('+')) != std::string::npos) {
            ret.resize(pos);
        }
        boost::trim(ret);
        LogPrint(BCLog::MERGEMINE, "Submitted work for chain 0x%s but user \"%s\" is not registered; assuming address is \"%s\".\n", HexStr(server.aux_pow_path), username, ret);
    }
    return ret;
}

void SubmitAuxChainShare(const uint256& chainid, const std::string& username, const AuxWork& work, const AuxProof& proof)
{
    LOCK(cs_mergemine);

    // Lookup the server corresponding to this chainid:
    AuxWorkServer* server = GetServerFromChainId(chainid, __func__);
    if (!server) {
        return;
    }

    // Lookup the registered address for the user.
    const std::string address = GetRegisteredAddress(*server, username);

    // Submit the share to the server:
    SendAuxSubmitRequest(*server, address, work, proof);
}

void SubmitSecondStageShare(const uint256& chainid, const std::string& username, const SecondStageWork& work, const SecondStageProof& proof)
{
    LOCK(cs_mergemine);

    // Lookup the server corresponding to this chainid:
    AuxWorkServer* server = GetServerFromChainId(chainid, __func__);
    if (!server) {
        return;
    }

    // Lookup the registered address for the user.
    const std::string address = GetRegisteredAddress(*server, username);

    SendSubmitRequest(*server, address, work, proof);
}

static UniValue stratum_mining_aux_notify(AuxWorkServer& server, const UniValue& params) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    const std::string method("mining.aux.notify");
    BoundParams(method, params, 5, 5);

    // Timestamp for the work
    uint64_t time = GetTimeMillis();

    // The job_id's internal structure is technically not defined.  For maximum
    // compatibility we make no assumptions and store it in the format in which
    // we received it: a string.
    std::string job_id = params[0].get_str();

    // The second parameter is the list of commitments for each registered miner
    // on this channel.  We'll come back to that.

    // The third parameter is the base difficulty of the share.
    uint32_t bits = ParseHexInt4(params[2], "nBits");

    // The fourth value is the share's bias setting.
    int bias = params[3].get_int();
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
    const std::string method("mining.notify");
    BoundParams(method, params, 8, 9);

    // Timestamp for the work
    uint64_t time = GetTimeMillis();

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
        std::string line(cstr, len);
        free(cstr);
        LogPrint(BCLog::MERGEMINE, "Received line of data from auxiliary work source stratum+tcp://%s (%s) : %s\n", server.socket.ToString(), server.name, line);

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
                const int id = val["id"].get_int();
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
            LogPrint(BCLog::MERGEMINE, "Received line of data from auxiliary work source stratum+tcp://%s (%s): %s\n", server.socket.ToString(), server.name, e.what());
            continue;
        }

        try {
            if (isreply && val["id"].get_int() == -1) {
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
                    extranonce2_size = result[2].get_int();
                } catch (...) {
                    // result[2] could not be cast to integer
                    throw std::runtime_error("expected integer extranonce2_size as third value of mining.subscribe response");
                }
                LogPrint(BCLog::MERGEMINE, "Setting extranonce1 to \"%s\" and extranonce2_size to %d from stratum+tcp://%s (%s)\n", HexStr(extranonce1), extranonce2_size, server.socket.ToString(), server.name);
                swap(server.extranonce1, extranonce1);
                server.extranonce2_size = extranonce2_size;
            }

            if (isreply && val["id"].get_int() == -2) {
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
                uint256 aux_pow_path;
                try {
                    aux_pow_path = ParseUInt256(result[0], "aux_pow_path");
                } catch (...) {
                    // result[0] was not a uint256
                    throw std::runtime_error("expected hex-encoded aux_pow_path as first value of mining.aux.subscribe response");
                }
                server.aux_pow_path = aux_pow_path;
                if (g_mergemine.count(aux_pow_path)) {
                    // Already have a source for this chain
                    throw std::runtime_error("already have auxiliary work source for this chain");
                }
                LogPrintf("Registering auxiliary work notifications for chain 0x%s from stratum+tcp://%s (%s)\n", HexStr(aux_pow_path), server.socket.ToString(), server.name);
                g_mergemine[aux_pow_path] = bev;
            }
        } catch (const std::exception& e) {
            LogPrint(BCLog::MERGEMINE, "Received %s response from stratum+tcp://%s (%s): %s\n", val["id"].get_int()==-1 ? "mining.subscribe" : "mining.aux.subscribe", server.socket.ToString(), server.name, e.what());
            LogPrintf("Unable to subscribe to auxiliary work notifications from stratum+tcp://%s (%s) ; not adding\n", server.socket.ToString(), server.name);
            // Do nothing here.  The connection will be torn down without the
            // server having been added as a source of auxiliary work units.
            done = true;
        }
        if (isreply) {
            int id = val["id"].get_int();
            if (server.aux_auth_jreqid.count(id)) {
                std::string username(server.aux_auth_jreqid[id]);
                std::string address(val["result"].get_str());
                LogPrint(BCLog::MERGEMINE, "Mapping username \"%s\" to remote address \"%s\" for stratum+tcp://%s (%s)\n", username, address, server.socket.ToString(), server.name);
                server.aux_auth[username] = address;
                server.aux_auth_jreqid.erase(id);
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
            reply = JSONRPCReply(NullUniValue, objError, jreq.id);
        } catch (const std::exception& e) {
            reply = JSONRPCReply(NullUniValue, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
        }

        LogPrint(BCLog::STRATUM, "Sending stratum response to stratum+tcp://%s (%s) : %s", server.socket.ToString(), server.name, reply);
        if (evbuffer_add(output, reply.data(), reply.size())) {
            LogPrint(BCLog::STRATUM, "Sending stratum response failed. (Reason: %d, '%s')\n", errno, evutil_socket_error_to_string(errno));
        }
    }

    if (done) {
        LogPrint(BCLog::MERGEMINE, "Closing initial stratum connection to stratum+tcp://%s (%s)\n", server.socket.ToString(), server.name);
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
        LogPrint(BCLog::MERGEMINE, "Error detected on initial stratum connection to stratum+tcp://%s (%s)\n", server.socket.ToString(), server.name);
    }
    if (what & BEV_EVENT_EOF) {
        LogPrint(BCLog::MERGEMINE, "Remote disconnect received on stratum connection to stratum+tcp://%s (%s)\n", server.socket.ToString(), server.name);
    }
    // Remove the connection from our records, and tell libevent to disconnect
    // and free its resources.
    if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        // Remove connection from g_mergemine.
        if (g_mergemine.count(server.aux_pow_path) && g_mergemine[server.aux_pow_path] == bev) {
            LogPrintf("Unregistering auxiliary work notifications for chain 0x%s from stratum+tcp://%s (%s)\n", HexStr(server.aux_pow_path), server.socket.ToString(), server.name);
            g_mergemine.erase(server.aux_pow_path);
        }
        // Add connection to g_mergemine_noconn.
        LogPrintf("Queing stratum+tcp://%s (%s) for later reconnect\n", server.socket.ToString(), server.name);
        g_mergemine_noconn[server.socket] = AuxServerDisconnect(server.name);
        // Remove connection from g_mergemine_conn.
        LogPrint(BCLog::MERGEMINE, "Closing initial stratum connection to stratum+tcp://%s (%s)\n", server.socket.ToString(), server.name);
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
        LogPrintf("Unable to connect to stratum+tcp://%s (%s)\n", socket.ToString(), name);
        bufferevent_free(bev);
        bev = nullptr;
        return nullptr;
    }

    return bev;
}

static void SendSubscribeRequest(AuxWorkServer& server) EXCLUSIVE_LOCKS_REQUIRED(cs_mergemine)
{
    LogPrintf("Sending request to source merge-mine work from stratum+tcp://%s (%s)\n", server.socket.ToString(), server.name);

    static const std::string request =
        "{\"id\":-1,\"method\":\"mining.subscribe\",\"params\":[\""
        + FormatFullVersion()
        + "\"]}\n{\"id\":-2,\"method\":\"mining.aux.subscribe\",\"params\":[]}\n";

    LogPrint(BCLog::MERGEMINE, "Sending stratum request to %s (%s) : %s", server.socket.ToString(), server.name, request);
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
        LogPrintf("Queing stratum+tcp://%s (%s) for later reconnect\n", socket.ToString(), conn.name);
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

static void LocalReconnectToMergeMineEndpoints()
{
    uint64_t now = GetTimeMillis();

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
        LogPrintf("Attempting reconnect to stratum+tcp://%s (%s)\n", socket.ToString(), conn.name);
        g_mergemine_noconn.erase(socket);
        bufferevent* bev = ConnectToStratumEndpoint(socket, conn);
    }
}

static int DefaultMergeMinePort()
{
    const std::string& name = gArgs.GetChainName();
    if (name == CBaseChainParams::REGTEST) {
        return 29638;
    }
    if (name == CBaseChainParams::TESTNET) {
        return 19638;
    }
    return 9638;
}

static std::atomic<bool> g_shutdown = false;
static Mutex g_shutdown_mutex;
static std::condition_variable g_shutdown_cv;
void MergeMiningManagerThread()
{
    int defaultPort = DefaultMergeMinePort();

    if (!gArgs.IsArgSet("-mergeminename")) {
        chain_names.clear();
        // Freicoin / Tradecraft main network.
        chain_names["freicoin"] = CreateChainParams(gArgs, CBaseChainParams::MAIN)->DefaultAuxPowPath();
        chain_names["tradecraft"] = chain_names["freicoin"];
        // Freicoin / Tradecraft test network.
        chain_names["testnet"] = CreateChainParams(gArgs, CBaseChainParams::TESTNET)->DefaultAuxPowPath();
        // Freicoin / Tradecraft RPC test network.
        chain_names["regtest"] = CreateChainParams(gArgs, CBaseChainParams::REGTEST)->DefaultAuxPowPath();
    } else {
        // Setup the text string -> aux_pow_path map from conf-file settings:
        const std::vector<std::string>& vnames = gArgs.GetArgs("-mergeminename");
        for (std::vector<std::string>::const_iterator i = vnames.begin(); i != vnames.end(); ++i) {
            size_t pos = i->find(':');
            if (pos == std::string::npos) {
                continue;
            }
            std::string name(*i, 0, pos);
            std::string chainid(*i, pos+1);
            uint256 hash;
            try {
                hash = ParseUInt256(chainid, "chainid");
            } catch (...) {
                LogPrintf("Unable to convert \"%s\" to uint256. Not a proper chain id?\n", chainid);
            }
            std::reverse(hash.begin(), hash.end());
            LogPrintf("Adding \"%s\" as alternative name for merge-mine chain id 0x%s\n", name, HexStr(hash));
            chain_names[name] = hash;
        }
    }

    if (gArgs.IsArgSet("-mergemine")) {
        // Servers which we will attempt to to establish a connection to.
        std::map<CService, std::string> servers;

        const std::vector<std::string>& vmerge = gArgs.GetArgs("-mergemine");
        for (std::vector<std::string>::const_iterator i = vmerge.begin(); i != vmerge.end(); ++i) {
            CService socket;
            if (!Lookup(i->c_str(), socket, defaultPort, true)) {
                LogPrintf("Invalid socket address for -mergemine endpoint: %s ; skipping\n", *i);
                continue;
            }

            if (servers.count(socket)) {
                LogPrintf("Duplicate -mergemine endpoint: %s (same as %s) ; skipping\n", *i, servers[socket]);
                continue;
            }

            servers[socket] = *i;
        }

        for (std::map<CService, std::string>::const_iterator i = servers.begin(); i != servers.end(); ++i) {
            const CService& socket = i->first;
            const std::string& name = i->second;
            ConnectToStratumEndpoint(socket, name);
            // Handle any events that have been triggered by our actions so far.
            event_base_loop(base, EVLOOP_NONBLOCK);
        }
    }

    LogPrint(BCLog::MERGEMINE, "Entering merge-mining event dispatch loop\n");
    while (!g_shutdown) {
        // Attempt to re-establish any connections that have been dropped.
        ReconnectToMergeMineEndpoints();

        // Enter event dispatch loop.
        event_base_dispatch(base);

        // Shutdown the thread if there are no connections to manage.
        g_shutdown = g_shutdown || (g_mergemine_conn.empty() && g_mergemine_noconn.empty());

        // If we are not done, wait 15 seconds before re-starting the dispatch
        // loop, to prevent us from spin-locking.
        if (!g_shutdown) {
            WAIT_LOCK(g_shutdown_mutex, lock);
            g_shutdown_cv.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::seconds(15));
        }
    }
    LogPrint(BCLog::MERGEMINE, "Exited merge-mining event dispatch loop\n");
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
    g_shutdown_cv.notify_all();
}

/** Cleanup network connections made by the merge-mining subsystem, free
 ** associated resources, and cleanup global state. */
void StopMergeMining()
{
    g_shutdown = true;
    g_shutdown_cv.notify_all();
    if (merge_mining_manager_thread.joinable()) {
        merge_mining_manager_thread.join();
    }
    LOCK(cs_mergemine);
    /* Tear-down active connections. */
    for (const auto& bind : g_mergemine) {
        const AuxWorkServer& server = g_mergemine_conn[bind.second];
        LogPrintf("Unregistering auxiliary work notifications for chain 0x%s from stratum+tcp://%s (%s)\n", HexStr(server.aux_pow_path), server.socket.ToString(), server.name);
    }
    g_mergemine.clear();
    for (auto& server : g_mergemine_conn) {
        LogPrint(BCLog::MERGEMINE, "Closing stratum connection to stratum+tcp://%s (%s) due to process termination\n", server.second.socket.ToString(), server.second.name);
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

// End of File
