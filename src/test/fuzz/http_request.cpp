// Copyright (c) 2020-2022 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <httpserver.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

extern "C" int evhttp_parse_firstline_(struct evhttp_request*, struct evbuffer*);
extern "C" int evhttp_parse_headers_(struct evhttp_request*, struct evbuffer*);

std::string RequestMethodString(HTTPRequest::RequestMethod m);

FUZZ_TARGET(http_request)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    evhttp_request* evreq = evhttp_request_new(nullptr, nullptr);
    assert(evreq != nullptr);
    evreq->kind = EVHTTP_REQUEST;
    evbuffer* evbuf = evbuffer_new();
    assert(evbuf != nullptr);
    const std::vector<uint8_t> http_buffer = ConsumeRandomLengthByteVector(fuzzed_data_provider, 4096);
    evbuffer_add(evbuf, http_buffer.data(), http_buffer.size());
    // Avoid constructing requests that will be interpreted by libevent as PROXY requests to avoid triggering
    // a nullptr dereference. The dereference (req->evcon->http_server) takes place in evhttp_parse_request_line
    // and is a consequence of our hacky but necessary use of the internal function evhttp_parse_firstline_ in
    // this fuzzing harness. The workaround is not aesthetically pleasing, but it successfully avoids the troublesome
    // code path. " http:// HTTP/1.1\n" was a crashing input prior to this workaround.
    const std::string http_buffer_str = ToLower(std::string{http_buffer.begin(), http_buffer.end()});
    if (http_buffer_str.find(" http://") != std::string::npos || http_buffer_str.find(" https://") != std::string::npos ||
        evhttp_parse_firstline_(evreq, evbuf) != 1 || evhttp_parse_headers_(evreq, evbuf) != 1) {
        evbuffer_free(evbuf);
        evhttp_request_free(evreq);
        return;
    }

    util::SignalInterrupt interrupt;
    HTTPRequest http_request{evreq, interrupt, true};
    const HTTPRequest::RequestMethod request_method = http_request.GetRequestMethod();
    (void)RequestMethodString(request_method);
    (void)http_request.GetURI();
    (void)http_request.GetHeader("Host");
    const std::string header = fuzzed_data_provider.ConsumeRandomLengthString(16);
    (void)http_request.GetHeader(header);
    (void)http_request.WriteHeader(header, fuzzed_data_provider.ConsumeRandomLengthString(16));
    (void)http_request.GetHeader(header);
    const std::string body = http_request.ReadBody();
    assert(body.empty());
    const CService service = http_request.GetPeer();
    assert(service.ToStringAddrPort() == "[::]:0");

    evbuffer_free(evbuf);
    evhttp_request_free(evreq);
}
