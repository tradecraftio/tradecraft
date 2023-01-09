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

#ifndef BITCOIN_IPC_PROTOCOL_H
#define BITCOIN_IPC_PROTOCOL_H

#include <interfaces/init.h>

#include <functional>
#include <memory>
#include <typeindex>

namespace ipc {
struct Context;

//! IPC protocol interface for calling IPC methods over sockets.
//!
//! There may be different implementations of this interface for different IPC
//! protocols (e.g. Cap'n Proto, gRPC, JSON-RPC, or custom protocols).
class Protocol
{
public:
    virtual ~Protocol() = default;

    //! Return Init interface that forwards requests over given socket descriptor.
    //! Socket communication is handled on a background thread.
    virtual std::unique_ptr<interfaces::Init> connect(int fd, const char* exe_name) = 0;

    //! Handle requests on provided socket descriptor, forwarding them to the
    //! provided Init interface. Socket communication is handled on the
    //! current thread, and this call blocks until the socket is closed.
    virtual void serve(int fd, const char* exe_name, interfaces::Init& init) = 0;

    //! Add cleanup callback to interface that will run when the interface is
    //! deleted.
    virtual void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) = 0;

    //! Context accessor.
    virtual Context& context() = 0;
};
} // namespace ipc

#endif // BITCOIN_IPC_PROTOCOL_H
