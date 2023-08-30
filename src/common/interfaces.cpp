// Copyright (c) 2021-2022 The Bitcoin Core developers
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

#include <interfaces/echo.h>
#include <interfaces/handler.h>

#include <boost/signals2/connection.hpp>
#include <memory>
#include <utility>

namespace common {
namespace {
class CleanupHandler : public interfaces::Handler
{
public:
    explicit CleanupHandler(std::function<void()> cleanup) : m_cleanup(std::move(cleanup)) {}
    ~CleanupHandler() override { if (!m_cleanup) return; m_cleanup(); m_cleanup = nullptr; }
    void disconnect() override { if (!m_cleanup) return; m_cleanup(); m_cleanup = nullptr; }
    std::function<void()> m_cleanup;
};

class SignalHandler : public interfaces::Handler
{
public:
    explicit SignalHandler(boost::signals2::connection connection) : m_connection(std::move(connection)) {}

    void disconnect() override { m_connection.disconnect(); }

    boost::signals2::scoped_connection m_connection;
};

class EchoImpl : public interfaces::Echo
{
public:
    std::string echo(const std::string& echo) override { return echo; }
};
} // namespace
} // namespace common

namespace interfaces {
std::unique_ptr<Handler> MakeCleanupHandler(std::function<void()> cleanup)
{
    return std::make_unique<common::CleanupHandler>(std::move(cleanup));
}

std::unique_ptr<Handler> MakeSignalHandler(boost::signals2::connection connection)
{
    return std::make_unique<common::SignalHandler>(std::move(connection));
}

std::unique_ptr<Echo> MakeEcho() { return std::make_unique<common::EchoImpl>(); }
} // namespace interfaces
