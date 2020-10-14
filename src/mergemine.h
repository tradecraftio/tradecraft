// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MERGEMINE_H
#define BITCOIN_MERGEMINE_H

#include <chainparams.h> // for ChainId
#include <node/context.h>
#include <sync.h>
#include <uint256.h>

#include <map>
#include <string>

//! Mapping of alternative names to chain specifiers
extern std::map<std::string, ChainId> chain_names;

//! Critical seciton guarding access to all of the merge-mining global state
extern RecursiveMutex cs_merge_mining;

/** Reconnect to any auxiliary work sources with dropped connections. */
void ReconnectToMergeMineEndpoints();

/** Configure the merge-mining subsystem.  This involved setting up some global
 ** state and spawning initialization and management threads. */
bool InitMergeMining(node::NodeContext& node);

/** Interrupt any active network connections. */
void InterruptMergeMining();

/** Cleanup network connections made by the merge-mining subsystem, free
 ** associated resources, and cleanup global state. */
void StopMergeMining();

#endif // BITCOIN_MERGEMINE_H

// End of File
