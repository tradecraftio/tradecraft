// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
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

#ifndef FREICOIN_POW_H
#define FREICOIN_POW_H

#include <consensus/consensus.h>
#include <consensus/params.h>

#include "primitives/block.h"

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

int64_t GetFilteredTime(const CBlockIndex* pindexLast, const Consensus::Params&);

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params&);

uint32_t GetNextWorkRequiredAux(const CBlockIndex* pindexLast, const CBlockHeader& block, const Consensus::Params&);
uint32_t CalculateNextWorkRequiredAux(const CBlockIndex* pindexLast, const Consensus::Params&);

/** Verify that a block's work target is within the range of half to
 ** twice the targets of the past 12 blocks. */
bool CheckNextWorkRequiredAux(const CBlockIndex* pindexLast, const CBlockHeader& block, const Consensus::Params&);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckAuxiliaryProofOfWork(const CBlockHeader& block, const Consensus::Params&);
bool CheckProofOfWork(const CBlockHeader& block, const Consensus::Params&);

#endif // FREICOIN_POW_H
