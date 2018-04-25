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

#ifndef FREICOIN_CONSENSUS_AMOUNT_H
#define FREICOIN_CONSENSUS_AMOUNT_H

#include <cstdint>

/** Amount in kria (Can be negative) */
typedef int64_t CAmount;

/** The amount of kria in one FRC. */
static constexpr CAmount COIN = 100000000;

/** No amount larger than this (in kria) is valid.
 *
 * Note that this constant is *not* the total money supply, which in Freicoin
 * currently happens to be less than 21,000,000 FRC for various reasons, but
 * rather a sanity check. As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 * */
static constexpr CAmount MAX_MONEY = 21000000 * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

/** Adjust to present value by subtracting (or adding) demurrage */
static const bool DEFAULT_DISABLE_TIME_ADJUST = false;
extern bool disable_time_adjust;
CAmount TimeAdjustValueForward(const CAmount& initial_value, uint32_t distance);
CAmount TimeAdjustValueReverse(const CAmount& initial_value, uint32_t distance);
CAmount GetTimeAdjustedValue(const CAmount& initial_value, int relative_depth);

/** Convert between demurrage currency and inflationary scrip. */
CAmount FreicoinToScrip(const CAmount& freicoin, uint32_t height);
CAmount ScripToFreicoin(const CAmount& scrip, uint32_t height);

#endif // FREICOIN_CONSENSUS_AMOUNT_H
