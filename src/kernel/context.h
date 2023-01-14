// Copyright (c) 2022 The Bitcoin Core developers
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

#ifndef BITCOIN_KERNEL_CONTEXT_H
#define BITCOIN_KERNEL_CONTEXT_H

#include <memory>

class ECCVerifyHandle;

namespace kernel {
//! Context struct holding the kernel library's logically global state, and
//! passed to external libbitcoin_kernel functions which need access to this
//! state. The kernel library API is a work in progress, so state organization
//! and member list will evolve over time.
//!
//! State stored directly in this struct should be simple. More complex state
//! should be stored to std::unique_ptr members pointing to opaque types.
struct Context {
    std::unique_ptr<ECCVerifyHandle> ecc_verify_handle;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the kernel::Context struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    Context();
    ~Context();
};
} // namespace kernel

#endif // BITCOIN_KERNEL_CONTEXT_H
