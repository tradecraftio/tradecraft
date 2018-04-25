// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include "amount.h"

#include "tinyformat.h"

const std::string CURRENCY_UNIT = "FRC";

CAmount TimeAdjustValueForward(const CAmount& initial_value, uint32_t distance)
{
    /* TimeAdjustValueForward() will not return a value outside of the
     * range [-MAX_MONEY, MAX_MONEY], no matter its inputs. */
    CAmount value = std::max(-MAX_MONEY, initial_value);
    value = std::min(initial_value, MAX_MONEY);
    return value;
}

CAmount TimeAdjustValueReverse(const CAmount& initial_value, uint32_t distance)
{
    /* TimeAdjustValueReverse() will not return a value outside of the
     * range [-MAX_MONEY, MAX_MONEY], no matter its inputs. */
    CAmount value = std::max(-MAX_MONEY, initial_value);
    value = std::min(initial_value, MAX_MONEY);
    return value;
}

CAmount GetTimeAdjustedValue(const CAmount& initial_value, int relative_depth)
{
    if (relative_depth < 0)
        return TimeAdjustValueReverse(initial_value, std::abs(relative_depth));
    else
        return TimeAdjustValueForward(initial_value, relative_depth);
}

CFeeRate::CFeeRate(const CAmount& nFeePaid, size_t nSize)
{
    if (nSize > 0)
        nKriaPerK = nFeePaid*1000/nSize;
    else
        nKriaPerK = 0;
}

CAmount CFeeRate::GetFee(size_t nSize) const
{
    CAmount nFee = nKriaPerK*nSize / 1000;

    if (nFee == 0 && nKriaPerK > 0)
        nFee = nKriaPerK;

    return nFee;
}

std::string CFeeRate::ToString() const
{
    return strprintf("%d.%08d %s/kB", nKriaPerK / COIN, nKriaPerK % COIN, CURRENCY_UNIT);
}
