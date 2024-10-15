// Copyright (c) 2024-present The Bitcoin Core developers
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

#include <logging.h>
#include <node/timeoffsets.h>
#include <node/warnings.h>
#include <sync.h>
#include <tinyformat.h>
#include <util/time.h>
#include <util/translation.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <limits>
#include <optional>

using namespace std::chrono_literals;

void TimeOffsets::Add(std::chrono::seconds offset)
{
    LOCK(m_mutex);

    if (m_offsets.size() >= MAX_SIZE) {
        m_offsets.pop_front();
    }
    m_offsets.push_back(offset);
    LogDebug(BCLog::NET, "Added time offset %+ds, total samples %d\n",
             Ticks<std::chrono::seconds>(offset), m_offsets.size());
}

std::chrono::seconds TimeOffsets::Median() const
{
    LOCK(m_mutex);

    // Only calculate the median if we have 5 or more offsets
    if (m_offsets.size() < 5) return 0s;

    auto sorted_copy = m_offsets;
    std::sort(sorted_copy.begin(), sorted_copy.end());
    return sorted_copy[sorted_copy.size() / 2];  // approximate median is good enough, keep it simple
}

bool TimeOffsets::WarnIfOutOfSync() const
{
    // when median == std::numeric_limits<int64_t>::min(), calling std::chrono::abs is UB
    auto median{std::max(Median(), std::chrono::seconds(std::numeric_limits<int64_t>::min() + 1))};
    if (std::chrono::abs(median) <= WARN_THRESHOLD) {
        m_warnings.Unset(node::Warning::CLOCK_OUT_OF_SYNC);
        return false;
    }

    bilingual_str msg{strprintf(_(
        "Your computer's date and time appear to be more than %d minutes out of sync with the network, "
        "this may lead to consensus failure. After you've confirmed your computer's clock, this message "
        "should no longer appear when you restart your node. Without a restart, it should stop showing "
        "automatically after you've connected to a sufficient number of new outbound peers, which may "
        "take some time. You can inspect the `timeoffset` field of the `getpeerinfo` and `getnetworkinfo` "
        "RPC methods to get more info."
    ), Ticks<std::chrono::minutes>(WARN_THRESHOLD))};
    LogWarning("%s\n", msg.original);
    m_warnings.Set(node::Warning::CLOCK_OUT_OF_SYNC, msg);
    return true;
}
