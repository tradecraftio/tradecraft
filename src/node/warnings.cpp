// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <node/warnings.h>

#include <common/system.h>
#include <node/interface_ui.h>
#include <sync.h>
#include <univalue.h>
#include <util/translation.h>

#include <utility>
#include <vector>

namespace node {
Warnings::Warnings()
{
    // Pre-release build warning
    if (!CLIENT_VERSION_IS_RELEASE) {
        m_warnings.insert(
            {Warning::PRE_RELEASE_TEST_BUILD,
             _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications")});
    }
}
bool Warnings::Set(warning_type id, bilingual_str message)
{
    const auto& [_, inserted]{WITH_LOCK(m_mutex, return m_warnings.insert({id, std::move(message)}))};
    if (inserted) uiInterface.NotifyAlertChanged();
    return inserted;
}

bool Warnings::Unset(warning_type id)
{
    auto success{WITH_LOCK(m_mutex, return m_warnings.erase(id))};
    if (success) uiInterface.NotifyAlertChanged();
    return success;
}

std::vector<bilingual_str> Warnings::GetMessages() const
{
    LOCK(m_mutex);
    std::vector<bilingual_str> messages;
    messages.reserve(m_warnings.size());
    for (const auto& [id, msg] : m_warnings) {
        messages.push_back(msg);
    }
    return messages;
}

UniValue GetWarningsForRpc(const Warnings& warnings, bool use_deprecated)
{
    if (use_deprecated) {
        const auto all_messages{warnings.GetMessages()};
        return all_messages.empty() ? "" : all_messages.back().original;
    }

    UniValue messages{UniValue::VARR};
    for (auto&& message : warnings.GetMessages()) {
        messages.push_back(std::move(message.original));
    }
    return messages;
}
} // namespace node
