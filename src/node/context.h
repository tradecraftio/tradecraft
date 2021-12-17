// Copyright (c) 2019 The Bitcoin Core developers
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

#ifndef BITCOIN_NODE_CONTEXT_H
#define BITCOIN_NODE_CONTEXT_H

#include <memory>
#include <vector>

class BanMan;
class CConnman;
class CScheduler;
class CTxMemPool;
class PeerLogicValidation;
namespace interfaces {
class Chain;
class ChainClient;
} // namespace interfaces

//! NodeContext struct containing references to chain state and connection
//! state.
//!
//! This is used by init, rpc, and test code to pass object references around
//! without needing to declare the same variables and parameters repeatedly, or
//! to use globals. More variables could be added to this struct (particularly
//! references to validation objects) to eliminate use of globals
//! and make code more modular and testable. The struct isn't intended to have
//! any member functions. It should just be a collection of references that can
//! be used without pulling in unwanted dependencies or functionality.
struct NodeContext {
    std::unique_ptr<CConnman> connman;
    CTxMemPool* mempool{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::unique_ptr<PeerLogicValidation> peer_logic;
    std::unique_ptr<BanMan> banman;
    std::unique_ptr<interfaces::Chain> chain;
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;
    std::unique_ptr<CScheduler> scheduler;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the NodeContext struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    NodeContext();
    ~NodeContext();
};

#endif // BITCOIN_NODE_CONTEXT_H
