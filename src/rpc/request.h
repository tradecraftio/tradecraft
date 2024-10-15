// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
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

#ifndef BITCOIN_RPC_REQUEST_H
#define BITCOIN_RPC_REQUEST_H

#include <any>
#include <optional>
#include <string>

#include <univalue.h>
#include <util/fs.h>

enum class JSONRPCVersion {
    V1_LEGACY,
    V2
};

/** JSON-RPC 2.0 request, only used in bitcoin-cli **/
UniValue JSONRPCRequestObj(const std::string& strMethod, const UniValue& params, const UniValue& id);
UniValue JSONRPCReplyObj(UniValue result, UniValue error, std::optional<UniValue> id, JSONRPCVersion jsonrpc_version);
UniValue JSONRPCError(int code, const std::string& message);

/** Generate a new RPC authentication cookie and write it to disk */
bool GenerateAuthCookie(std::string* cookie_out, std::optional<fs::perms> cookie_perms=std::nullopt);
/** Read the RPC authentication cookie from disk */
bool GetAuthCookie(std::string *cookie_out);
/** Delete RPC authentication cookie from disk */
void DeleteAuthCookie();
/** Parse JSON-RPC batch reply into a vector */
std::vector<UniValue> JSONRPCProcessBatchReply(const UniValue& in);

class JSONRPCRequest
{
public:
    std::optional<UniValue> id = UniValue::VNULL;
    std::string strMethod;
    UniValue params;
    enum Mode { EXECUTE, GET_HELP, GET_ARGS } mode = EXECUTE;
    std::string URI;
    std::string authUser;
    std::string peerAddr;
    std::any context;
    JSONRPCVersion m_json_version = JSONRPCVersion::V1_LEGACY;

    void parse(const UniValue& valRequest);
    [[nodiscard]] bool IsNotification() const { return !id.has_value() && m_json_version == JSONRPCVersion::V2; };
};

#endif // BITCOIN_RPC_REQUEST_H
