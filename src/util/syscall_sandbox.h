// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#ifndef BITCOIN_UTIL_SYSCALL_SANDBOX_H
#define BITCOIN_UTIL_SYSCALL_SANDBOX_H

enum class SyscallSandboxPolicy {
    // 1. Initialization
    INITIALIZATION,
    INITIALIZATION_DNS_SEED,
    INITIALIZATION_LOAD_BLOCKS,
    INITIALIZATION_MAP_PORT,

    // 2. Steady state (non-initialization, non-shutdown)
    MESSAGE_HANDLER,
    NET,
    NET_ADD_CONNECTION,
    NET_HTTP_SERVER,
    NET_HTTP_SERVER_WORKER,
    NET_OPEN_CONNECTION,
    SCHEDULER,
    TOR_CONTROL,
    TX_INDEX,
    VALIDATION_SCRIPT_CHECK,

    // 3. Shutdown
    SHUTOFF,
};

//! Force the current thread (and threads created from the current thread) into a restricted-service
//! operating mode where only a subset of all syscalls are available.
//!
//! Subsequent calls to this function can reduce the abilities further, but abilities can never be
//! regained.
//!
//! This function is a no-op unless SetupSyscallSandbox(...) has been called.
//!
//! SetupSyscallSandbox(...) is called during bitcoind initialization if Bitcoin Core was compiled
//! with seccomp-bpf support (--with-seccomp) *and* the parameter -sandbox=<mode> was passed to
//! bitcoind.
//!
//! This experimental feature is available under Linux x86_64 only.
void SetSyscallSandboxPolicy(SyscallSandboxPolicy syscall_policy);

#if defined(USE_SYSCALL_SANDBOX)
//! Setup and enable the experimental syscall sandbox for the running process.
[[nodiscard]] bool SetupSyscallSandbox(bool log_syscall_violation_before_terminating);

//! Invoke a disallowed syscall. Use for testing purposes.
void TestDisallowedSandboxCall();
#endif // defined(USE_SYSCALL_SANDBOX)

#endif // BITCOIN_UTIL_SYSCALL_SANDBOX_H
