#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
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

import threading
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import random_bytes


class NetDeadlockTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        self.log.info("Simultaneously send a large message on both sides")
        rand_msg = random_bytes(4000000).hex()

        thread0 = threading.Thread(target=node0.sendmsgtopeer, args=(0, "unknown", rand_msg))
        thread1 = threading.Thread(target=node1.sendmsgtopeer, args=(0, "unknown", rand_msg))

        thread0.start()
        thread1.start()
        thread0.join()
        thread1.join()

        self.log.info("Check whether a deadlock happened")
        self.generate(node0, 1)
        self.sync_blocks()


if __name__ == '__main__':
    NetDeadlockTest().main()
