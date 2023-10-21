// Copyright (c) 2020-2023 The Freicoin Developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
