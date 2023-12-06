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

#include <chain.h>
#include <interfaces/chain.h>
#include <kernel/chain.h>
#include <sync.h>
#include <uint256.h>

class CBlock;

namespace kernel {
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* index, const CBlock* data)
{
    interfaces::BlockInfo info{index ? *index->phashBlock : uint256::ZERO};
    if (index) {
        info.prev_hash = index->pprev ? index->pprev->phashBlock : nullptr;
        info.height = index->nHeight;
        info.chain_time_max = index->GetBlockTimeMax();
        LOCK(::cs_main);
        info.file_number = index->nFile;
        info.data_pos = index->nDataPos;
    }
    info.data = data;
    return info;
}
} // namespace kernel

std::ostream& operator<<(std::ostream& os, const ChainstateRole& role) {
    switch(role) {
        case ChainstateRole::NORMAL: os << "normal"; break;
        case ChainstateRole::ASSUMEDVALID: os << "assumedvalid"; break;
        case ChainstateRole::BACKGROUND: os << "background"; break;
        default: os.setstate(std::ios_base::failbit);
    }
    return os;
}
