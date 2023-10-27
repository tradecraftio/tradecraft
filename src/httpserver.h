// Copyright (c) 2015-2020 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
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

#ifndef BITCOIN_HTTPSERVER_H
#define BITCOIN_HTTPSERVER_H

#include <string>
#include <functional>

#include <netbase.h>

static const int DEFAULT_HTTP_THREADS=4;
static const int DEFAULT_HTTP_WORKQUEUE=16;
static const int DEFAULT_HTTP_SERVER_TIMEOUT=30;

struct evhttp_request;
struct event_base;
class CService;
class HTTPRequest;

/** Check if a network address is allowed to access the server */
bool ClientAllowed(const std::vector<CSubNet>& allowed_subnets, const CNetAddr& netaddr);

/** Initialize ACL list for HTTP server */
bool InitSubnetAllowList(const std::string which, std::vector<CSubNet>& allowed_subnets);

/** Determine what addresses to bind to. */
bool InitEndpointList(const std::string& which, uint16_t default_port, std::vector<std::pair<std::string, uint16_t> >& endpoints);

/** Initialize HTTP server.
 * Call this before RegisterHTTPHandler or EventBase().
 */
bool InitHTTPServer();
/** Start HTTP server.
 * This is separate from InitHTTPServer to give users race-condition-free time
 * to register their handlers between InitHTTPServer and StartHTTPServer.
 */
void StartHTTPServer();
/** Interrupt HTTP server threads */
void InterruptHTTPServer();
/** Stop HTTP server */
void StopHTTPServer();

/** Change logging level for libevent. Removes BCLog::LIBEVENT from log categories if
 * libevent doesn't support debug logging.*/
bool UpdateHTTPServerLogging(bool enable);

/** Handler for requests to a certain HTTP path */
typedef std::function<bool(HTTPRequest* req, const std::string &)> HTTPRequestHandler;
/** Register handler for prefix.
 * If multiple handlers match a prefix, the first-registered one will
 * be invoked.
 */
void RegisterHTTPHandler(const std::string &prefix, bool exactMatch, const HTTPRequestHandler &handler);
/** Unregister handler for prefix */
void UnregisterHTTPHandler(const std::string &prefix, bool exactMatch);

/** Return evhttp event base. This can be used by submodules to
 * queue timers or custom events.
 */
struct event_base* EventBase();

/** In-flight HTTP request.
 * Thin C++ wrapper around evhttp_request.
 */
class HTTPRequest
{
private:
    struct evhttp_request* req;
    bool replySent;

public:
    explicit HTTPRequest(struct evhttp_request* req, bool replySent = false);
    ~HTTPRequest();

    enum RequestMethod {
        UNKNOWN,
        GET,
        POST,
        HEAD,
        PUT
    };

    /** Get requested URI.
     */
    std::string GetURI() const;

    /** Get CService (address:ip) for the origin of the http request.
     */
    CService GetPeer() const;

    /** Get request method.
     */
    RequestMethod GetRequestMethod() const;

    /**
     * Get the request header specified by hdr, or an empty string.
     * Return a pair (isPresent,string).
     */
    std::pair<bool, std::string> GetHeader(const std::string& hdr) const;

    /**
     * Read request body.
     *
     * @note As this consumes the underlying buffer, call this only once.
     * Repeated calls will return an empty string.
     */
    std::string ReadBody();

    /**
     * Write output header.
     *
     * @note call this before calling WriteErrorReply or Reply.
     */
    void WriteHeader(const std::string& hdr, const std::string& value);

    /**
     * Write HTTP reply.
     * nStatus is the HTTP status code to send.
     * strReply is the body of the reply. Keep it empty to send a standard message.
     *
     * @note Can be called only once. As this will give the request back to the
     * main thread, do not call any other HTTPRequest methods after calling this.
     */
    void WriteReply(int nStatus, const std::string& strReply = "");
};

/** Event handler closure.
 */
class NetEventClosure
{
public:
    virtual void operator()() = 0;
    virtual ~NetEventClosure() {}
};

/** Event class. This can be used either as a cross-thread trigger or as a timer.
 */
class HTTPEvent
{
public:
    /** Create a new event.
     * deleteWhenTriggered deletes this event object after the event is triggered (and the handler called)
     * handler is the handler to call when the event is triggered.
     */
    HTTPEvent(struct event_base* base, bool deleteWhenTriggered, const std::function<void()>& handler);
    ~HTTPEvent();

    /** Trigger the event. If tv is 0, trigger it immediately. Otherwise trigger it after
     * the given time has elapsed.
     */
    void trigger(struct timeval* tv);

    bool deleteWhenTriggered;
    std::function<void()> handler;
private:
    struct event* ev;
};

#endif // BITCOIN_HTTPSERVER_H
