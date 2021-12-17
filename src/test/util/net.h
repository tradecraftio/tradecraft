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

#ifndef BITCOIN_TEST_UTIL_NET_H
#define BITCOIN_TEST_UTIL_NET_H

#include <net.h>

struct ConnmanTestMsg : public CConnman {
    using CConnman::CConnman;
    void AddTestNode(CNode& node)
    {
        LOCK(cs_vNodes);
        vNodes.push_back(&node);
    }
    void ClearTestNodes()
    {
        LOCK(cs_vNodes);
        for (CNode* node : vNodes) {
            delete node;
        }
        vNodes.clear();
    }

    void ProcessMessagesOnce(CNode& node) { m_msgproc->ProcessMessages(&node, flagInterruptMsgProc); }

    void NodeReceiveMsgBytes(CNode& node, const char* pch, unsigned int nBytes, bool& complete) const;

    bool ReceiveMsgFrom(CNode& node, CSerializedNetMsg& ser_msg) const;
};

#endif // BITCOIN_TEST_UTIL_NET_H
