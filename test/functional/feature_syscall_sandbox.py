#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Copyright (c) 2010-2023 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of version 3 of the GNU Affero General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
# details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""Test freicoind aborts if a disallowed syscall is used when compiled with the syscall sandbox."""

from test_framework.test_framework import FreicoinTestFramework, SkipTest


class SyscallSandboxTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        if not self.is_syscall_sandbox_compiled():
            raise SkipTest("freicoind has not been built with syscall sandbox enabled.")
        if self.disable_syscall_sandbox:
            raise SkipTest("--nosandbox passed to test runner.")

    def run_test(self):
        disallowed_syscall_terminated_freicoind = False
        expected_log_entry = 'ERROR: The syscall "getgroups" (syscall number 115) is not allowed by the syscall sandbox'
        with self.nodes[0].assert_debug_log([expected_log_entry]):
            self.log.info("Invoking disallowed syscall")
            try:
                self.nodes[0].invokedisallowedsyscall()
            except ConnectionError:
                disallowed_syscall_terminated_freicoind = True
        assert disallowed_syscall_terminated_freicoind
        self.nodes = []


if __name__ == "__main__":
    SyscallSandboxTest().main()
