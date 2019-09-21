#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Bitcoin Core developers
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
"""Test the generation of UTXO snapshots using `dumptxoutset`.
"""

from test_framework.blocktools import COINBASE_MATURITY
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
        self.generate(node, COINBASE_MATURITY)

        FILENAME = 'txoutset.dat'
        out = node.dumptxoutset(FILENAME)
        expected_path = Path(node.datadir) / self.chain / FILENAME

        assert expected_path.is_file()

        assert_equal(out['coins_written'], 101)
        assert_equal(out['base_height'], 100)
        assert_equal(out['path'], str(expected_path))
        # Blockhash should be deterministic based on mocked time.
        assert_equal(
            out['base_hash'],
            '7dc3398f11755b2089cda332744d49ce6d546e402de2cb6e65a254b30936296e')

        with open(str(expected_path), 'rb') as f:
            digest = hashlib.sha256(f.read()).hexdigest()
            # UTXO snapshot hash should be deterministic based on mocked time.
            assert_equal(
                digest, '1b2331cc7cc68be8a7297a26ad282261a44378e0ac3f58d11b9aa8c6fee7d177')

        assert_equal(
            out['txoutset_hash'], '477629b2b2ceece9822a2a16348c9a56461871d51ebc2c9223a56ed98a8858fc')
        assert_equal(out['nchaintx'], 101)

        # Specifying a path to an existing file will fail.
        assert_raises_rpc_error(
            -8, '{} already exists'.format(FILENAME),  node.dumptxoutset, FILENAME)

if __name__ == '__main__':
    DumptxoutsetTest().main()
