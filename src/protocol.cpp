// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
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

#include "protocol.h"

#include "chainparams.h"
#include "consensus/consensus.h"
#include "consensus/params.h"
#include "timedata.h"
#include "util.h"
#include "utilstrencodings.h"

#include <algorithm>
#include <limits>

#ifndef WIN32
# include <arpa/inet.h>
#endif

namespace NetMsgType {
const char *VERSION="version";
const char *VERACK="verack";
const char *ADDR="addr";
const char *INV="inv";
const char *GETDATA="getdata";
const char *MERKLEBLOCK="merkleblock";
const char *GETBLOCKS="getblocks";
const char *GETHEADERS="getheaders";
const char *TX="tx";
const char *HEADERS="headers";
const char *BLOCK="block";
const char *GETADDR="getaddr";
const char *MEMPOOL="mempool";
const char *PING="ping";
const char *PONG="pong";
const char *ALERT="alert";
const char *NOTFOUND="notfound";
const char *FILTERLOAD="filterload";
const char *FILTERADD="filteradd";
const char *FILTERCLEAR="filterclear";
const char *REJECT="reject";
const char *SENDHEADERS="sendheaders";
};

static const char* ppszTypeName[] =
{
    "ERROR", // Should never occur
    NetMsgType::TX,
    NetMsgType::BLOCK,
    "filtered block" // Should never occur
};

/** All known message types. Keep this in the same order as the list of
 * messages above and in protocol.h.
 */
const static std::string allNetMessageTypes[] = {
    NetMsgType::VERSION,
    NetMsgType::VERACK,
    NetMsgType::ADDR,
    NetMsgType::INV,
    NetMsgType::GETDATA,
    NetMsgType::MERKLEBLOCK,
    NetMsgType::GETBLOCKS,
    NetMsgType::GETHEADERS,
    NetMsgType::TX,
    NetMsgType::HEADERS,
    NetMsgType::BLOCK,
    NetMsgType::GETADDR,
    NetMsgType::MEMPOOL,
    NetMsgType::PING,
    NetMsgType::PONG,
    NetMsgType::ALERT,
    NetMsgType::NOTFOUND,
    NetMsgType::FILTERLOAD,
    NetMsgType::FILTERADD,
    NetMsgType::FILTERCLEAR,
    NetMsgType::REJECT,
    NetMsgType::SENDHEADERS
};
const static std::vector<std::string> allNetMessageTypesVec(allNetMessageTypes, allNetMessageTypes+ARRAYLEN(allNetMessageTypes));

extern int nMaxConnections; // declared in net.h
size_t MaxProtocolMessageLength(const Consensus::Params &params)
{
    // Unconstraining the block size in the protocol cleanup fork
    // means that network message size must also be unconstrained,
    // which is a potential DoS vector. Unfortunately there is no
    // easy way around this. Until better tools are available in
    // future versions, we must accept that after activation of
    // the protocol cleanup fork we might receive a message up to
    // the largest possible block size, which is limited only by
    // PROTOCOL_CLEANUP_MAX_BLOCKFILE_SIZE, which is nearly 2 GiB.
    //
    // However this value is dangerously high for 32-bit clients,
    // as it presents an easy DoS vector for memory exhaustion
    // attacks. We therefore use a lower limit for 32-bit builds
    // which prevents exhaustion of the memory address space with
    // the maximum number of connected peers. This does mean that
    // 32-bit clients will stop being able to synchronize from the
    // network once blocks genuinely grow larger than 16 MiB. But
    // as it is doubtful that a true 32-bit peer could keep up
    // with the network in such an instance, this is deemed an
    // acceptable tradeoff.
    size_t max_msg_size = MAX_PROTOCOL_MESSAGE_LENGTH;
    if (GetAdjustedTime() > (Params().GetConsensus().protocol_cleanup_activation_time - 2*60*60 /* two hours */)) {
        // Use no more than 2 GiB for messages in flight on 32-bit
        // peers. With the default max of 125 connections this is
        // slightly more than 16 MiB. A 32-bit node operator could
        // indirectly raise this value by lowering the maximum
        // number of allowed connections in their configuration
        // file. But we will not decrease below this amount just
        // because user configured their node to accept more
        // inbound peers than the default.
        size_t max_data_per_peer = std::numeric_limits<size_t>::max() / std::max(nMaxConnections, 125) / 2;
        // On 64-bit nodes, the above calculation results in an
        // enormous number, so we use the lower implicit protocol
        // rule of the maximum blockfile size--a block larger than
        // this value could not be stored to disk.
        max_msg_size = std::min(max_data_per_peer, static_cast<size_t>(PROTOCOL_CLEANUP_MAX_BLOCKFILE_SIZE - 8 + 24));
    }
    return max_msg_size;
}

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    nMessageSize = -1;
    nChecksum = 0;
}

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    strncpy(pchCommand, pszCommand, COMMAND_SIZE);
    nMessageSize = nMessageSizeIn;
    nChecksum = 0;
}

std::string CMessageHeader::GetCommand() const
{
    return std::string(pchCommand, pchCommand + strnlen(pchCommand, COMMAND_SIZE));
}

bool CMessageHeader::IsValid(const MessageStartChars& pchMessageStartIn) const
{
    // Check start string
    if (memcmp(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE) != 0)
        return false;

    // Check the command string for errors
    for (const char* p1 = pchCommand; p1 < pchCommand + COMMAND_SIZE; p1++)
    {
        if (*p1 == 0)
        {
            // Must be all zeros after the first zero
            for (; p1 < pchCommand + COMMAND_SIZE; p1++)
                if (*p1 != 0)
                    return false;
        }
        else if (*p1 < ' ' || *p1 > 0x7E)
            return false;
    }

    // Message size
    if (nMessageSize > MaxProtocolMessageLength(Params().GetConsensus()))
    {
        LogPrintf("CMessageHeader::IsValid(): (%s, %u bytes) nMessageSize > MAX_SIZE\n", GetCommand(), nMessageSize);
        return false;
    }

    return true;
}



CAddress::CAddress() : CService()
{
    Init();
}

CAddress::CAddress(CService ipIn, uint64_t nServicesIn) : CService(ipIn)
{
    Init();
    nServices = nServicesIn;
}

void CAddress::Init()
{
    nServices = NODE_NETWORK;
    nTime = 100000000;
}

CInv::CInv()
{
    type = 0;
    hash.SetNull();
}

CInv::CInv(int typeIn, const uint256& hashIn)
{
    type = typeIn;
    hash = hashIn;
}

CInv::CInv(const std::string& strType, const uint256& hashIn)
{
    unsigned int i;
    for (i = 1; i < ARRAYLEN(ppszTypeName); i++)
    {
        if (strType == ppszTypeName[i])
        {
            type = i;
            break;
        }
    }
    if (i == ARRAYLEN(ppszTypeName))
        throw std::out_of_range(strprintf("CInv::CInv(string, uint256): unknown type '%s'", strType));
    hash = hashIn;
}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

bool CInv::IsKnownType() const
{
    return (type >= 1 && type < (int)ARRAYLEN(ppszTypeName));
}

const char* CInv::GetCommand() const
{
    if (!IsKnownType())
        throw std::out_of_range(strprintf("CInv::GetCommand(): type=%d unknown type", type));
    return ppszTypeName[type];
}

std::string CInv::ToString() const
{
    return strprintf("%s %s", GetCommand(), hash.ToString());
}

const std::vector<std::string> &getAllNetMessageTypes()
{
    return allNetMessageTypesVec;
}
