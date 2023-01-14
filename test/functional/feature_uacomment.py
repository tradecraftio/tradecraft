#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
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
"""Test the -uacomment option."""

import re

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework.util import assert_equal


class UacommentTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.log.info("test multiple -uacomment")
        test_uacomment = self.nodes[0].getnetworkinfo()["subversion"][-12:-1]
        assert_equal(test_uacomment, "(testnode0)")

        self.restart_node(0, ["-uacomment=foo"])
        foo_uacomment = self.nodes[0].getnetworkinfo()["subversion"][-17:-1]
        assert_equal(foo_uacomment, "(testnode0; foo)")

        self.log.info("test -uacomment max length")
        self.stop_node(0)
        expected = r"Error: Total length of network version string \([0-9]+\) exceeds maximum length \(256\). Reduce the number or size of uacomments."
        self.nodes[0].assert_start_raises_init_error(["-uacomment=" + 'a' * 256], expected, match=ErrorMatch.FULL_REGEX)

        self.log.info("test -uacomment unsafe characters")
        for unsafe_char in ['/', ':', '(', ')', '₿', '🏃']:
            expected = r"Error: User Agent comment \(" + re.escape(unsafe_char) + r"\) contains unsafe characters."
            self.nodes[0].assert_start_raises_init_error(["-uacomment=" + unsafe_char], expected, match=ErrorMatch.FULL_REGEX)


if __name__ == '__main__':
    UacommentTest().main()
