// Copyright (c) 2019-2021 The Bitcoin Core developers
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

#include <test/util/logging.h>

#include <logging.h>
#include <noui.h>
#include <tinyformat.h>

#include <stdexcept>

DebugLogHelper::DebugLogHelper(std::string message, MatchFn match)
    : m_message{std::move(message)}, m_match(std::move(match))
{
    m_print_connection = LogInstance().PushBackCallback(
        [this](const std::string& s) {
            if (m_found) return;
            m_found = s.find(m_message) != std::string::npos && m_match(&s);
        });
    noui_test_redirect();
}

void DebugLogHelper::check_found()
{
    noui_reconnect();
    LogInstance().DeleteCallback(m_print_connection);
    if (!m_found && m_match(nullptr)) {
        throw std::runtime_error(strprintf("'%s' not found in debug log\n", m_message));
    }
}
