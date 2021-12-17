// Copyright (c) 2020 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <test/util/net.h>

#include <chainparams.h>
#include <net.h>

void ConnmanTestMsg::NodeReceiveMsgBytes(CNode& node, const char* pch, unsigned int nBytes, bool& complete) const
{
    assert(node.ReceiveMsgBytes(pch, nBytes, complete));
    if (complete) {
        size_t nSizeAdded = 0;
        auto it(node.vRecvMsg.begin());
        for (; it != node.vRecvMsg.end(); ++it) {
            // vRecvMsg contains only completed CNetMessage
            // the single possible partially deserialized message are held by TransportDeserializer
            nSizeAdded += it->m_raw_message_size;
        }
        {
            LOCK(node.cs_vProcessMsg);
            node.vProcessMsg.splice(node.vProcessMsg.end(), node.vRecvMsg, node.vRecvMsg.begin(), it);
            node.nProcessQueueSize += nSizeAdded;
            node.fPauseRecv = node.nProcessQueueSize > nReceiveFloodSize;
        }
    }
}

bool ConnmanTestMsg::ReceiveMsgFrom(CNode& node, CSerializedNetMsg& ser_msg) const
{
    std::vector<unsigned char> ser_msg_header;
    node.m_serializer->prepareForTransport(ser_msg, ser_msg_header);

    bool complete;
    NodeReceiveMsgBytes(node, (const char*)ser_msg_header.data(), ser_msg_header.size(), complete);
    NodeReceiveMsgBytes(node, (const char*)ser_msg.data.data(), ser_msg.data.size(), complete);
    return complete;
}
