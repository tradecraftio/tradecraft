// Copyright (c) 2021 The Bitcoin Core developers
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

#ifndef FREICOIN_IPC_CONTEXT_H
#define FREICOIN_IPC_CONTEXT_H

namespace ipc {
//! Context struct used to give IPC protocol implementations or implementation
//! hooks access to application state, in case they need to run extra code that
//! isn't needed within a single process, like code copying global state from an
//! existing process to a new process when it's initialized, or code dealing
//! with shared objects that are created or destroyed remotely.
struct Context
{
};
} // namespace ipc

#endif // FREICOIN_IPC_CONTEXT_H
