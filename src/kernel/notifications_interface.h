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

#ifndef FREICOIN_KERNEL_NOTIFICATIONS_INTERFACE_H
#define FREICOIN_KERNEL_NOTIFICATIONS_INTERFACE_H

#include <util/translation.h>

#include <cstdint>
#include <string>
#include <variant>

class CBlockIndex;
enum class SynchronizationState;

namespace kernel {

//! Result type for use with std::variant to indicate that an operation should be interrupted.
struct Interrupted{};

//! Simple result type for functions that need to propagate an interrupt status and don't have other return values.
using InterruptResult = std::variant<std::monostate, Interrupted>;

template <typename T>
bool IsInterrupted(const T& result)
{
    return std::holds_alternative<kernel::Interrupted>(result);
}

/**
 * A base class defining functions for notifying about certain kernel
 * events.
 */
class Notifications
{
public:
    virtual ~Notifications(){};

    [[nodiscard]] virtual InterruptResult blockTip(SynchronizationState state, CBlockIndex& index) { return {}; }
    virtual void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) {}
    virtual void progress(const bilingual_str& title, int progress_percent, bool resume_possible) {}
    virtual void warning(const bilingual_str& warning) {}

    //! The flush error notification is sent to notify the user that an error
    //! occurred while flushing block data to disk. Kernel code may ignore flush
    //! errors that don't affect the immediate operation it is trying to
    //! perform. Applications can choose to handle the flush error notification
    //! by logging the error, or notifying the user, or triggering an early
    //! shutdown as a precaution against causing more errors.
    virtual void flushError(const std::string& debug_message) {}

    //! The fatal error notification is sent to notify the user when an error
    //! occurs in kernel code that can't be recovered from. After this
    //! notification is sent, whatever function triggered the error should also
    //! return an error code or raise an exception. Applications can choose to
    //! handle the fatal error notification by logging the error, or notifying
    //! the user, or triggering an early shutdown as a precaution against
    //! causing more errors.
    virtual void fatalError(const std::string& debug_message, const bilingual_str& user_message = {}) {}
};
} // namespace kernel

#endif // FREICOIN_KERNEL_NOTIFICATIONS_INTERFACE_H
