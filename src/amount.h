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

#ifndef FREICOIN_AMOUNT_H
#define FREICOIN_AMOUNT_H

#include <stdint.h>

/** Amount in kria (Can be negative) */
typedef int64_t CAmount;

static const CAmount COIN = 100000000;

/** No amount larger than this (in kria) is valid.
 *
 * Note that this constant is *not* the total money supply, which in Freicoin
 * currently happens to be more than this value for various reasons, but rather
 * a sanity check.  As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 *
 * Note also that UNLIKE bitcoin, this is less than the total monetary supply in
 * Freicoin.  It *is* possible to create a transaction which includes inputs
 * which exceed this value when combined together.  Such a transaction would be
 * invalid.
 */
static const CAmount MAX_MONEY = 9007199254740991LL; // 2^53 - 1
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

#endif //  FREICOIN_AMOUNT_H
