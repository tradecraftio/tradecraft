// Copyright (c) 2023 The Bitcoin Core developers
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

#ifndef FREICOIN_NODE_KERNEL_NOTIFICATIONS_H
#define FREICOIN_NODE_KERNEL_NOTIFICATIONS_H

#include <kernel/notifications_interface.h>

#include <atomic>
#include <cstdint>
#include <string>

class ArgsManager;
class CBlockIndex;
enum class SynchronizationState;
struct bilingual_str;

namespace node {

static constexpr int DEFAULT_STOPATHEIGHT{0};

class KernelNotifications : public kernel::Notifications
{
public:
    KernelNotifications(std::atomic<int>& exit_status) : m_exit_status{exit_status} {}

    [[nodiscard]] kernel::InterruptResult blockTip(SynchronizationState state, CBlockIndex& index) override;

    void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override;

    void progress(const bilingual_str& title, int progress_percent, bool resume_possible) override;

    void warning(const bilingual_str& warning) override;

    void flushError(const std::string& debug_message) override;

    void fatalError(const std::string& debug_message, const bilingual_str& user_message = {}) override;

    //! Block height after which blockTip notification will return Interrupted{}, if >0.
    int m_stop_at_height{DEFAULT_STOPATHEIGHT};
    //! Useful for tests, can be set to false to avoid shutdown on fatal error.
    bool m_shutdown_on_fatal_error{true};
private:
    std::atomic<int>& m_exit_status;
};

void ReadNotificationArgs(const ArgsManager& args, KernelNotifications& notifications);

} // namespace node

#endif // FREICOIN_NODE_KERNEL_NOTIFICATIONS_H
