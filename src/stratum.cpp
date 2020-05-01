// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum.h>

#include <chainparams.h>
#include <chainparamsbase.h>
#include <common/args.h>
#include <httpserver.h>
#include <logging.h>
#include <netbase.h>
#include <node/context.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/check.h>

#include <string>
#include <vector>

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
#endif

//! The time at which the statum server started, used to calculate uptime.
static int64_t g_start_time = 0;

//! Reference to the NodeContext for the process
static node::NodeContext* g_context;

//! List of subnets to allow stratum connections from
static std::vector<CSubNet> stratum_allow_subnets;

//! Bound stratum listening sockets
static std::map<evconnlistener*, CService> bound_listeners;

/** Callback to accept a stratum connection. */
static void stratum_accept_conn_cb(evconnlistener *listener, evutil_socket_t fd, sockaddr *address, int socklen, void *ctx)
{
    CService service;
    service.SetSockAddr(address);
    LogPrint(BCLog::STRATUM, "Accepted stratum connection from %s\n", service.ToStringAddrPort());
}

/** Setup the stratum connection listening services */
static bool StratumBindAddresses(event_base* base, node::NodeContext& node)
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
        CService socket(LookupHost(endpoint.first.c_str(), true).value(), endpoint.second);
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

/** Configure the stratum server */
bool InitStratumServer(node::NodeContext& node)
{
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
    g_start_time = GetTime();

    return true;
}

/** Interrupt the stratum server connections */
void InterruptStratumServer()
{
    // Stop listening for connections on stratum sockets
    for (const auto& binding : bound_listeners) {
        LogPrint(BCLog::STRATUM, "Interrupting stratum service on %s\n", binding.second.ToStringAddrPort());
        evconnlistener_disable(binding.first);
    }
}

/** Cleanup stratum server network connections and free resources. */
void StopStratumServer()
{
    /* Un-bind our listeners from their network interfaces. */
    for (const auto& binding : bound_listeners) {
        LogPrint(BCLog::STRATUM, "Removing stratum server binding on %s\n", binding.second.ToStringAddrPort());
        evconnlistener_free(binding.first);
    }
    bound_listeners.clear();
}

static RPCHelpMan getstratuminfo() {
    return RPCHelpMan{"getstratuminfo",
        "\nReturns an object containing various state info regarding the stratum server.\n",
        {},
        RPCResult{RPCResult::Type::OBJ, "", "", {
            {RPCResult::Type::BOOL, "enabled", "whether the server is running or not"},
            {RPCResult::Type::NUM_TIME, "curtime", /*optional=*/true, "the current time, in seconds since UNIX epoch"},
            {RPCResult::Type::NUM_TIME, "uptime", /*optional=*/true, "the server uptime, in seconds"},
            {RPCResult::Type::ARR, "listeners", /*optional=*/true, "network interfaces the server is listening on", {
                {RPCResult::Type::OBJ, "", "", {
                    {RPCResult::Type::STR, "netaddr", "the network interface address and port"},
                }},
            }},
            {RPCResult::Type::ARR, "allowip", /*optional=*/true, "subnets the server is allowed to accept connections from", {
                {RPCResult::Type::STR, "subnet", "the subnet"},
            }},
        }},
        RPCExamples{
            HelpExampleCli("getstratuminfo", "")
            + HelpExampleRpc("getstratuminfo", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Check if the stratum server is even enabled.
    // If not, our response is short.
    UniValue ret(UniValue::VOBJ);
    bool enabled = !bound_listeners.empty();
    ret.pushKV("enabled", enabled);
    if (!enabled) {
        return ret;
    }
    int64_t now = GetTime();
    ret.pushKV("curtime", now);
    ret.pushKV("uptime", now - g_start_time);
    // Report which network interfaces we are listening on.
    UniValue listeners(UniValue::VARR);
    for (const auto& binding : bound_listeners) {
        UniValue listener(UniValue::VOBJ);
        listener.pushKV("netaddr", binding.second.ToStringAddrPort());
        listeners.push_back(listener);
    }
    ret.pushKV("listeners", listeners);
    // Report which IP subnets we are alloweding connections from.
    UniValue allowed(UniValue::VARR);
    for (const auto& subnet : stratum_allow_subnets) {
        allowed.push_back(subnet.ToString());
    }
    ret.pushKV("allowip", allowed);
    return ret;
},
    };
}

void RegisterStratumRPCCommands(CRPCTable& t) {
    static const CRPCCommand commands[]{
        {"mining", &getstratuminfo},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

// End of File
