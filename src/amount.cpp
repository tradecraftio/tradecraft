// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include "amount.h"

#include "tinyformat.h"

const std::string CURRENCY_UNIT = "FRC";

/** Only set to true when running the regtest chain with the
 ** '-notimeadjust' option set, making TimeAdjustValueForward() and
 ** TimeAdjustValueBackward() return their inputs unmodified. This
 ** enables running bitcoin regression tests unmodified. */
bool disable_time_adjust = DEFAULT_DISABLE_TIME_ADJUST;

CAmount TimeAdjustValueForward(const CAmount& initial_value, uint32_t distance)
{
    /* If we're in bitcoin unit test compatibility mode, return our
     * input unmodified, with no demurrage adjustment. */
    if (disable_time_adjust)
        return initial_value;

    /* TimeAdjustValueForward() will not return a value outside of the
     * range [-MAX_MONEY, MAX_MONEY], no matter its inputs. */
    CAmount value = std::max(-MAX_MONEY, initial_value);
    value = std::min(initial_value, MAX_MONEY);
    return value;
}

CAmount TimeAdjustValueReverse(const CAmount& initial_value, uint32_t distance)
{
    /* If we're in bitcoin unit test compatibility mode, return our
     * input unmodified, with no demurrage adjustment. */
    if (disable_time_adjust)
        return initial_value;

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

CFeeRate::CFeeRate(const CAmount& nFeePaid, size_t nBytes_)
{
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    if (nSize > 0)
        nKriaPerK = nFeePaid * 1000 / nSize;
    else
        nKriaPerK = 0;
}

CAmount CFeeRate::GetFee(size_t nBytes_) const
{
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    CAmount nFee = nKriaPerK * nSize / 1000;

    if (nFee == 0 && nSize != 0) {
        if (nKriaPerK > 0)
            nFee = CAmount(1);
        if (nKriaPerK < 0)
            nFee = CAmount(-1);
    }

    return nFee;
}

std::string CFeeRate::ToString() const
{
    return strprintf("%d.%08d %s/kB", nKriaPerK / COIN, nKriaPerK % COIN, CURRENCY_UNIT);
}
