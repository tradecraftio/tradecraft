// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
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

#ifndef FREICOIN_INIT_H
#define FREICOIN_INIT_H

#include <any>
#include <memory>
#include <string>

//! Default value for -daemon option
static constexpr bool DEFAULT_DAEMON = false;
//! Default value for -daemonwait option
static constexpr bool DEFAULT_DAEMONWAIT = false;

class ArgsManager;
struct NodeContext;
namespace interfaces {
struct BlockAndHeaderTipInfo;
}

/** Interrupt threads */
void Interrupt(NodeContext& node);
void Shutdown(NodeContext& node);
//!Initialize the logging infrastructure
void InitLogging(const ArgsManager& args);
//!Parameter interaction: change current parameters depending on various rules
void InitParameterInteraction(ArgsManager& args);

/** Initialize freicoin: Basic context setup.
 *  @note This can be done before daemonization. Do not call Shutdown() if this function fails.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInitBasicSetup(const ArgsManager& args);
/**
 * Initialization: parameter interaction.
 * @note This can be done before daemonization. Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitBasicSetup should have been called.
 */
bool AppInitParameterInteraction(const ArgsManager& args);
/**
 * Initialization sanity checks: ecc init, sanity checks, dir lock.
 * @note This can be done before daemonization. Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitParameterInteraction should have been called.
 */
bool AppInitSanityChecks();
/**
 * Lock freicoin data directory.
 * @note This should only be done after daemonization. Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitSanityChecks should have been called.
 */
bool AppInitLockDataDirectory();
/**
 * Initialize node and wallet interface pointers. Has no prerequisites or side effects besides allocating memory.
 */
bool AppInitInterfaces(NodeContext& node);
/**
 * Freicoin main initialization.
 * @note This should only be done after daemonization. Call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitLockDataDirectory should have been called.
 */
bool AppInitMain(NodeContext& node, interfaces::BlockAndHeaderTipInfo* tip_info = nullptr);

/**
 * Register all arguments with the ArgsManager
 */
void SetupServerArgs(ArgsManager& argsman);

/** Returns licensing information (for -version) */
std::string LicenseInfo();

#endif // FREICOIN_INIT_H
