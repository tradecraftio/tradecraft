#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Bitcoin Core developers
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
"""Create a blockchain cache.

Creating a cache of the blockchain speeds up test execution when running
multiple functional tests. This helper script is executed by test_runner when multiple
tests are being run in parallel.
"""

from test_framework.test_framework import FreicoinTestFramework

class CreateCache(FreicoinTestFramework):
    # Test network and test nodes are not required:

    def set_test_params(self):
        self.num_nodes = 0

    def setup_network(self):
        pass

    def run_test(self):
        pass

if __name__ == '__main__':
    CreateCache().main()
