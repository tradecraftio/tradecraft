#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Copyright (c) 2010-2022 The Freicoin Developers
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
"""Test the generation of UTXO snapshots using `dumptxoutset`.
"""
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

import hashlib
from pathlib import Path


class DumptxoutsetTest(FreicoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Test a trivial usage of the dumptxoutset RPC command."""
        node = self.nodes[0]
        mocktime = node.getblockheader(node.getblockhash(0))['time'] + 1
        node.setmocktime(mocktime)
        node.generate(100)

        FILENAME = 'txoutset.dat'
        out = node.dumptxoutset(FILENAME)
        expected_path = Path(node.datadir) / self.chain / FILENAME

        assert expected_path.is_file()

        assert_equal(out['coins_written'], 201)
        assert_equal(out['base_height'], 100)
        assert_equal(out['path'], str(expected_path))
        # Blockhash should be deterministic based on mocked time.
        assert_equal(
            out['base_hash'],
            '0386813308fa7573075988f57d6b5b301c1c2b7022fde15ab51c18755fba2344')

        with open(str(expected_path), 'rb') as f:
            digest = hashlib.sha256(f.read()).hexdigest()
            # UTXO snapshot hash should be deterministic based on mocked time.
            assert_equal(
                digest, '1e8133623f0720fe1efe2a42f32d0c78b2e4514fcf44bf6395848a306124d96c')

        # Specifying a path to an existing file will fail.
        assert_raises_rpc_error(
            -8, '{} already exists'.format(FILENAME),  node.dumptxoutset, FILENAME)

if __name__ == '__main__':
    DumptxoutsetTest().main()
