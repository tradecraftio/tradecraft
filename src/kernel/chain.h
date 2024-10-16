// Copyright (c) 2022 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#ifndef FREICOIN_KERNEL_CHAIN_H
#define FREICOIN_KERNEL_CHAIN_H

#include<iostream>

class CBlock;
class CBlockIndex;
namespace interfaces {
struct BlockInfo;
} // namespace interfaces

namespace kernel {
//! Return data from block index.
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* block_index, const CBlock* data = nullptr);

} // namespace kernel

//! This enum describes the various roles a specific Chainstate instance can take.
//! Other parts of the system sometimes need to vary in behavior depending on the
//! existence of a background validation chainstate, e.g. when building indexes.
enum class ChainstateRole {
    // Single chainstate in use, "normal" IBD mode.
    NORMAL,

    // Doing IBD-style validation in the background. Implies use of an assumed-valid
    // chainstate.
    BACKGROUND,

    // Active assumed-valid chainstate. Implies use of a background IBD chainstate.
    ASSUMEDVALID,
};

std::ostream& operator<<(std::ostream& os, const ChainstateRole& role);

#endif // FREICOIN_KERNEL_CHAIN_H
