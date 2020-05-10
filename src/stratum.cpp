// Copyright (c) 2020 The Freicoin Developers
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
#include "rpcserver.h"
#include "util.h"
#include "utilstrencodings.h"
#include "serialize.h"
#include "streams.h"
#include "sync.h"
#include "txmempool.h"

#include <univalue.h>

#include <algorithm> // for std::reverse
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp> // for boost::trim
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

struct StratumClient
{
    evconnlistener* m_listener;
    evutil_socket_t m_socket;
    bufferevent* m_bev;
    CService m_from;
    uint256 m_secret;

    CService GetPeer() const
      { return m_from; }

    std::string m_client;

    bool m_authorized;
    CFreicoinAddress m_addr;
    double m_mindiff;

    uint32_t m_version_rolling_mask;

    CBlockIndex* m_last_tip;
    bool m_send_work;

    StratumClient() : m_listener(0), m_socket(0), m_bev(0), m_authorized(false), m_mindiff(0.0), m_version_rolling_mask(0x00000000), m_last_tip(0), m_send_work(false) { GenSecret(); }
    StratumClient(evconnlistener* listener, evutil_socket_t socket, bufferevent* bev, CService from) : m_listener(listener), m_socket(socket), m_bev(bev), m_from(from), m_authorized(false), m_mindiff(0.0), m_version_rolling_mask(0x00000000), m_last_tip(0), m_send_work(false) { GenSecret(); }

    void GenSecret();
};

void StratumClient::GenSecret()
{
    GetRandBytes(m_secret.begin(), 32);
}

struct StratumWork {
    CBlockTemplate m_block_template;
    std::vector<uint256> m_cb_branch;

    StratumWork() { };
    StratumWork(const CBlockTemplate& block_template);

    CBlock& GetBlock()
      { return m_block_template.block; }
    const CBlock& GetBlock() const
      { return m_block_template.block; }
};

StratumWork::StratumWork(const CBlockTemplate& block_template)
    : m_block_template(block_template)
{
    std::vector<uint256> leaves;
    BOOST_FOREACH (const CTransaction& tx, m_block_template.block.vtx) {
        leaves.push_back(tx.GetHash());
    }
    m_cb_branch = ComputeMerkleBranch(leaves, 0);
};

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

uint32_t ParseHexInt4(UniValue hex, std::string name)
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

std::string GetWorkUnit(StratumClient& client)
{
    using std::swap;

    LOCK(cs_main);

    if (vNodes.empty() && !Params().MineBlocksOnDemand()) {
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Freicoin is not connected!");
    }

    if (IsInitialBlockDownload()) {
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Freicoin is downloading blocks...");
    }

    if (!client.m_authorized) {
        throw JSONRPCError(RPC_INVALID_REQUEST, "Stratum client not authorized.  Use mining.authorize first, with a Freicoin address as the username.");
    }

    static CBlockIndex* tip = NULL;
    static uint256 job_id;
    static unsigned int transactions_updated_last = 0;
    static int64_t last_update_time = 0;

    if (tip != chainActive.Tip() || (mempool.GetTransactionsUpdated() != transactions_updated_last && (GetTime() - last_update_time) > 5) || !work_templates.count(job_id))
    {
        CBlockIndex *tip_new = chainActive.Tip();
        const CScript script = CScript() << OP_TRUE;
        CBlockTemplate *new_work = CreateNewBlock(Params(), script);
        if (!new_work) {
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
        }
        // So that block.GetHash() is correct
        new_work->block.hashMerkleRoot = BlockMerkleRoot(new_work->block);

        job_id = new_work->block.GetHash();
        work_templates[job_id] = StratumWork(*new_work);
        tip = tip_new;

        transactions_updated_last = mempool.GetTransactionsUpdated();
        last_update_time = GetTime();

        delete new_work;
        new_work = NULL;

        LogPrint("stratum", "New stratum block template (%d total): %s\n", work_templates.size(), job_id.GetHex());

        // Remove any old templates
        std::vector<uint256> old_job_ids;
        boost::optional<uint256> oldest_job_id = boost::none;
        uint32_t oldest_job_nTime = last_update_time;
        typedef std::pair<uint256, StratumWork> template_type;
        BOOST_FOREACH (template_type work_template, work_templates) {
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
        BOOST_FOREACH (uint256 old_job_id, old_job_ids) {
            work_templates.erase(old_job_id);
            LogPrint("stratum", "Removed outdated stratum block template (%d total): %s\n", work_templates.size(), old_job_id.GetHex());
        }
        // Remove the oldest work unit if we're still over the maximum
        // number of stored work templates.
        if (work_templates.size() > 30 && oldest_job_id) {
            work_templates.erase(oldest_job_id.get());
            LogPrint("stratum", "Removed oldest stratum block template (%d total): %s\n", work_templates.size(), oldest_job_id.get().GetHex());
        }
    }

    StratumWork& current_work = work_templates[job_id];

    CBlockIndex tmp_index;
    tmp_index.nBits = current_work.GetBlock().nBits;
    double diff = GetDifficulty(&tmp_index);
    if (client.m_mindiff > 0) {
        diff = std::min(diff, client.m_mindiff);
    }
    diff = std::max(diff, 0.001);

    UniValue set_difficulty(UniValue::VOBJ);
    set_difficulty.push_back(Pair("id", NullUniValue));
    set_difficulty.push_back(Pair("method", "mining.set_difficulty"));
    UniValue set_difficulty_params(UniValue::VARR);
    set_difficulty_params.push_back(diff);
    set_difficulty.push_back(Pair("params", set_difficulty_params));

    CMutableTransaction cbmtx(current_work.GetBlock().vtx[0]);
    uint256 job_nonce;
    CSHA256()
        .Write(client.m_secret.begin(), 32)
        .Write(job_id.begin(), 32)
        .Finalize(job_nonce.begin());
    std::vector<unsigned char> nonce(job_nonce.begin(),
                                     job_nonce.begin()+8);
    nonce.resize(nonce.size()+4, 0x00);
    cbmtx.vin.front().scriptSig =
           CScript()
        << cbmtx.lock_height
        << nonce;
    cbmtx.vout.front().scriptPubKey =
        GetScriptForDestination(client.m_addr.Get());
    CDataStream cb(SER_GETHASH, PROTOCOL_VERSION);
    CTransaction cbtx(cbmtx);
    cb << cbtx;
    assert(cb.size() >= (4 + 1 + 32 + 4 + 1));
    size_t pos = 4 + 1 + 32 + 4 + 1 + cb[4+1+32+4] - 4;
    assert(cb.size() >= (pos + 4));

    std::string cb1 = HexStr(&cb[0], &cb[pos]);
    std::string cb2 = HexStr(&cb[pos+4], &cb[cb.size()]);

    UniValue params(UniValue::VARR);
    params.push_back(job_id.GetHex());
    // For reasons of who-the-heck-knows-why, stratum byte-swaps each
    // 32-bit chunk of the hashPrevBlock, and prints in reverse order.
    // The byte swaps are only done with this hash.
    uint256 hashPrevBlock(current_work.GetBlock().hashPrevBlock);
    for (int i = 0; i < 256/32; ++i) {
        ((uint32_t*)hashPrevBlock.begin())[i] = bswap_32(
            ((uint32_t*)hashPrevBlock.begin())[i]);
    }
    std::reverse(hashPrevBlock.begin(),
                 hashPrevBlock.end());
    params.push_back(hashPrevBlock.GetHex());
    params.push_back(cb1);
    params.push_back(cb2);

    std::vector<uint256> vbranch = current_work.m_cb_branch;
    // Reverse the order of the hashes, because that's what stratum does.
    for (int j = 0; j < vbranch.size(); ++j) {
        std::reverse(vbranch[j].begin(),
                     vbranch[j].end());
    }

    UniValue branch(UniValue::VARR);
    BOOST_FOREACH (const uint256& hash, vbranch) {
        branch.push_back(hash.GetHex());
    }
    params.push_back(branch);

    CBlockHeader blkhdr(current_work.GetBlock());
    int64_t delta = UpdateTime(&blkhdr, Params().GetConsensus(), tip);
    LogPrint("stratum", "Updated the timestamp of block template by %d seconds\n", delta);

    params.push_back(HexInt4(blkhdr.nVersion));
    params.push_back(HexInt4(blkhdr.nBits));
    params.push_back(HexInt4(blkhdr.nTime));
    params.push_back(client.m_last_tip != tip);
    client.m_last_tip = tip;

    UniValue mining_notify(UniValue::VOBJ);
    mining_notify.push_back(Pair("params", params));
    mining_notify.push_back(Pair("id", NullUniValue));
    mining_notify.push_back(Pair("method", "mining.notify"));

    return set_difficulty.write() + "\n"
         + mining_notify.write()  + "\n";
}

bool SubmitBlock(StratumClient& client, const uint256& job_id, const StratumWork& current_work, std::vector<unsigned char> extranonce2, uint32_t nTime, uint32_t nNonce, uint32_t nVersion)
{
    assert(current_work.GetBlock().vtx.size() >= 1);
    CMutableTransaction cb(current_work.GetBlock().vtx.front());
    assert(cb.vin.size() == 1);
    uint256 job_nonce;
    CSHA256()
        .Write(client.m_secret.begin(), 32)
        .Write(job_id.begin(), 32)
        .Finalize(job_nonce.begin());
    std::vector<unsigned char> nonce(job_nonce.begin(),
                                     job_nonce.begin()+8);
    assert(extranonce2.size() == 4);
    nonce.insert(nonce.end(), extranonce2.begin(),
                              extranonce2.end());
    cb.vin.front().scriptSig =
           CScript()
        << cb.lock_height
        << nonce;
    assert(cb.vout.size() >= 1);
    cb.vout.front().scriptPubKey =
        GetScriptForDestination(client.m_addr.Get());

    CBlockHeader blkhdr(current_work.GetBlock());
    blkhdr.hashMerkleRoot = ComputeMerkleRootFromBranch(cb.GetHash(), current_work.m_cb_branch, 0);
    blkhdr.nTime = nTime;
    blkhdr.nNonce = nNonce;
    blkhdr.nVersion = nVersion;

    bool res = false;
    if (CheckProofOfWork(blkhdr.GetHash(), blkhdr.nBits, Params().GetConsensus())) {
        LogPrintf("GOT BLOCK!!! by %s: %s\n", client.m_addr.ToString(), blkhdr.GetHash().ToString());
        CBlock block(current_work.GetBlock());
        block.vtx.front() = CTransaction(cb);
        block.hashMerkleRoot = BlockMerkleRoot(block);
        block.nTime = nTime;
        block.nNonce = nNonce;
        block.nVersion = nVersion;
        CValidationState state;
        res = ProcessNewBlock(state, Params(), NULL, &block, true, NULL);
    } else {
        LogPrintf("NEW SHARE!!! by %s: %s\n", client.m_addr.ToString(), blkhdr.GetHash().ToString());
    }

    client.m_send_work = true;

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

    UniValue ret(UniValue::VARR);

    UniValue notify(UniValue::VARR);
    notify.push_back("mining.notify");
    notify.push_back("ae6812eb4cd7735a302a8a9dd95cf71f");
    ret.push_back(notify);

    ret.push_back(""); //        extranonce1
    ret.push_back(4);  // sizeof(extranonce2)

    //ScheduleSendWork(client);
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
            client.m_version_rolling_mask = mask;
            res.push_back(Pair("version-rolling", true));
            res.push_back(Pair("version-rolling.mask", HexInt4(mask & 0x1fffffff)));
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
    BoundParams(method, params, 5, 6);

    std::string username = params[0].get_str();
    boost::trim(username);

    // There may or may not be a '+' suffix in the username, so we
    // clean it up just in case:
    size_t pos = username.find('+');
    if (pos != std::string::npos) {
        username.resize(pos);
        boost::trim_right(username);
    }

    uint256 job_id = uint256S(params[1].get_str());
    if (!work_templates.count(job_id)) {
        LogPrint("stratum", "Received completed share for unknown job_id : %s\n", job_id.GetHex());
        return true;
    }
    StratumWork &current_work = work_templates[job_id];

    std::vector<unsigned char> extranonce2 = ParseHexV(params[2], "extranonce2");
    assert(extranonce2.size() == 4);
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
    while (cstr = evbuffer_readln(input, &len, EVBUFFER_EOL_CRLF)) {
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
            reply = JSONRPCReply(NullUniValue, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
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
            data = JSONRPCReply(NullUniValue, JSONRPCError(RPC_PARSE_ERROR, e.what()), NullUniValue);
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
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
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
bool StratumBindAddresses(event_base* base)
{
    int defaultPort = GetArg("-stratumport", BaseParams().StratumPort());
    std::vector<std::pair<std::string, uint16_t> > endpoints;

    // Determine what addresses to bind to
    if (!InitEndpointList("stratum", defaultPort, endpoints))
        return false;

    // Bind each addresses
    for (std::vector<std::pair<std::string, uint16_t> >::iterator i = endpoints.begin(); i != endpoints.end(); ++i) {
        LogPrint("stratum", "Binding stratum on address %s port %i\n", i->first, i->second);
        // Use CService to translate string -> sockaddr
        CService socket(CNetAddr(i->first), i->second);
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
            LogPrintf("Binding stratum on address %s port %i failed. (Reason: %d, '%s')\n", i->first, i->second, errno, evutil_socket_error_to_string(errno));
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
        typedef std::pair<bufferevent*, StratumClient> subscription_type;
        BOOST_FOREACH (subscription_type subscription, subscriptions) {
            bufferevent* bev = subscription.first;
            evbuffer *output = bufferevent_get_output(bev);
            StratumClient& client = subscription.second;
            // Ignore clients that aren't authorized yet.
            if (!client.m_authorized) {
                continue;
            }
            // Get new work
            std::string data;
            try {
                data = GetWorkUnit(client);
            } catch (...) {
                // Some sort of error.  Ignore.
                LogPrint("stratum", "Error generating updated work for stratum client\n");
                continue;
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
    BOOST_FOREACH (const CSubNet& subnet, stratum_allow_subnets)
        strAllowed += subnet.ToString() + " ";
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
    typedef std::pair<evconnlistener*, CService> binding_type;
    BOOST_FOREACH (binding_type binding, bound_listeners) {
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
    typedef std::pair<bufferevent*, StratumClient> subscription_type;
    BOOST_FOREACH (subscription_type subscription, subscriptions) {
        LogPrint("stratum", "Closing stratum server connection to %s due to process termination\n", subscription.second.GetPeer().ToString());
        bufferevent_free(subscription.first);
    }
    subscriptions.clear();
    /* Un-bind our listeners from their network interfaces. */
    typedef std::pair<evconnlistener*, CService> binding_type;
    BOOST_FOREACH (binding_type binding, bound_listeners) {
        LogPrint("stratum", "Removing stratum server binding on %s\n", binding.second.ToString());
        evconnlistener_free(binding.first);
    }
    bound_listeners.clear();
    /* Free any allocated block templates. */
    work_templates.clear();
}

// End of File
