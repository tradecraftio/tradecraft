// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <test/util/net.h>

#include <chainparams.h>
#include <net.h>
#include <span.h>

#include <vector>

void ConnmanTestMsg::NodeReceiveMsgBytes(CNode& node, Span<const uint8_t> msg_bytes, bool& complete) const
{
    assert(node.ReceiveMsgBytes(msg_bytes, complete));
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
    std::vector<uint8_t> ser_msg_header;
    node.m_serializer->prepareForTransport(ser_msg, ser_msg_header);

    bool complete;
    NodeReceiveMsgBytes(node, ser_msg_header, complete);
    NodeReceiveMsgBytes(node, ser_msg.data, complete);
    return complete;
}

std::vector<NodeEvictionCandidate> GetRandomNodeEvictionCandidates(int n_candidates, FastRandomContext& random_context)
{
    std::vector<NodeEvictionCandidate> candidates;
    for (int id = 0; id < n_candidates; ++id) {
        candidates.push_back({
            /*id=*/id,
            /*m_connected=*/std::chrono::seconds{random_context.randrange(100)},
            /*m_min_ping_time=*/std::chrono::microseconds{random_context.randrange(100)},
            /*m_last_block_time=*/std::chrono::seconds{random_context.randrange(100)},
            /*m_last_tx_time=*/std::chrono::seconds{random_context.randrange(100)},
            /*fRelevantServices=*/random_context.randbool(),
            /*fRelayTxes=*/random_context.randbool(),
            /*fBloomFilter=*/random_context.randbool(),
            /*nKeyedNetGroup=*/random_context.randrange(100),
            /*prefer_evict=*/random_context.randbool(),
            /*m_is_local=*/random_context.randbool(),
            /*m_network=*/ALL_NETWORKS[random_context.randrange(ALL_NETWORKS.size())],
        });
    }
    return candidates;
}
