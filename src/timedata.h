// Copyright (c) 2014-2018 The Bitcoin Core developers
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

#ifndef FREICOIN_TIMEDATA_H
#define FREICOIN_TIMEDATA_H

#include <algorithm>
#include <assert.h>
#include <stdint.h>
#include <vector>

static const int64_t DEFAULT_MAX_TIME_ADJUSTMENT = 70 * 60;

class CNetAddr;

/**
 * Median filter over a stream of values.
 * Returns the median of the last N numbers
 */
template <typename T>
class CMedianFilter
{
private:
    std::vector<T> vValues;
    std::vector<T> vSorted;
    unsigned int nSize;

public:
    CMedianFilter(unsigned int _size, T initial_value) : nSize(_size)
    {
        vValues.reserve(_size);
        vValues.push_back(initial_value);
        vSorted = vValues;
    }

    void input(T value)
    {
        if (vValues.size() == nSize) {
            vValues.erase(vValues.begin());
        }
        vValues.push_back(value);

        vSorted.resize(vValues.size());
        std::copy(vValues.begin(), vValues.end(), vSorted.begin());
        std::sort(vSorted.begin(), vSorted.end());
    }

    T median() const
    {
        int vSortedSize = vSorted.size();
        assert(vSortedSize > 0);
        if (vSortedSize & 1) // Odd number of elements
        {
            return vSorted[vSortedSize / 2];
        } else // Even number of elements
        {
            return (vSorted[vSortedSize / 2 - 1] + vSorted[vSortedSize / 2]) / 2;
        }
    }

    int size() const
    {
        return vValues.size();
    }

    std::vector<T> sorted() const
    {
        return vSorted;
    }
};

/** Functions to keep track of adjusted P2P time */
int64_t GetTimeOffset();
int64_t GetAdjustedTime();
void AddTimeData(const CNetAddr& ip, int64_t nTime);

/**
 * Reset the internal state of GetTimeOffset(), GetAdjustedTime() and AddTimeData().
 */
void TestOnlyResetTimeData();

#endif // FREICOIN_TIMEDATA_H
