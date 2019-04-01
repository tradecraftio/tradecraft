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

#ifndef BITCOIN_INIT_H
#define BITCOIN_INIT_H

#include <string>

class CScheduler;
class CWallet;

namespace boost
{
class thread_group;
} // namespace boost

void StartShutdown();
bool ShutdownRequested();
/** Interrupt threads */
void Interrupt(boost::thread_group& threadGroup);
void Shutdown();
//!Initialize the logging infrastructure
void InitLogging();
//!Parameter interaction: change current parameters depending on various rules
void InitParameterInteraction();
bool AppInit2(boost::thread_group& threadGroup, CScheduler& scheduler);

/** The help message mode determines what help message to show */
enum HelpMessageMode {
    HMM_BITCOIND,
    HMM_BITCOIN_QT
};

/** Help for options shared between UI and daemon (for -help) */
std::string HelpMessage(HelpMessageMode mode);
/** Returns licensing information (for -version) */
std::string LicenseInfo();

#endif // BITCOIN_INIT_H
