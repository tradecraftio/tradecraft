// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
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

#ifndef BITCOIN_SHUTDOWN_H
#define BITCOIN_SHUTDOWN_H

#include <util/translation.h> // For bilingual_str

/** Abort with a message */
bool AbortNode(const std::string& strMessage, bilingual_str user_message = bilingual_str{});

/** Initialize shutdown state. This must be called before using either StartShutdown(),
 * AbortShutdown() or WaitForShutdown(). Calling ShutdownRequested() is always safe.
 */
bool InitShutdownState();

/** Request shutdown of the application. */
void StartShutdown();

/** Clear shutdown flag. Only use this during init (before calling WaitForShutdown in any
 * thread), or in the unit tests. Calling it in other circumstances will cause a race condition.
 */
void AbortShutdown();

/** Returns true if a shutdown is requested, false otherwise. */
bool ShutdownRequested();

/** Wait for StartShutdown to be called in any thread. This can only be used
 * from a single thread.
 */
void WaitForShutdown();

#endif // BITCOIN_SHUTDOWN_H
