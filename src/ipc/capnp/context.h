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

#ifndef FREICOIN_IPC_CAPNP_CONTEXT_H
#define FREICOIN_IPC_CAPNP_CONTEXT_H

#include <ipc/context.h>

namespace ipc {
namespace capnp {
//! Cap'n Proto context struct. Generally the parent ipc::Context struct should
//! be used instead of this struct to give all IPC protocols access to
//! application state, so there aren't unnecessary differences between IPC
//! protocols. But this specialized struct can be used to pass capnp-specific
//! function and object types to capnp hooks.
struct Context : ipc::Context
{
};
} // namespace capnp
} // namespace ipc

#endif // FREICOIN_IPC_CAPNP_CONTEXT_H
