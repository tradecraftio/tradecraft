#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
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
"""Test signet miner tool"""

import os.path
import subprocess
import sys
import time

from test_framework.key import ECKey
from test_framework.script_util import key_to_p2wpkh_script
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet_util import bytes_to_wif


CHALLENGE_PRIVATE_KEY = (42).to_bytes(32, 'big')


class SignetMinerTest(FreicoinTestFramework):
    def set_test_params(self):
        self.chain = "signet"
        self.setup_clean_chain = True
        self.num_nodes = 1

        # generate and specify signet challenge (simple p2wpkh script)
        privkey = ECKey()
        privkey.set(CHALLENGE_PRIVATE_KEY, True)
        pubkey = privkey.get_pubkey().get_bytes()
        challenge = key_to_p2wpkh_script(pubkey)
        self.extra_args = [[f'-signetchallenge={challenge.hex()}']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_cli()
        self.skip_if_no_wallet()
        self.skip_if_no_freicoin_util()

    def run_test(self):
        node = self.nodes[0]
        # import private key needed for signing block
        node.importprivkey(bytes_to_wif(CHALLENGE_PRIVATE_KEY))

        # generate block with signet miner tool
        base_dir = self.config["environment"]["SRCDIR"]
        signet_miner_path = os.path.join(base_dir, "contrib", "signet", "miner")
        subprocess.run([
                sys.executable,
                signet_miner_path,
                f'--cli={node.cli.binary} -datadir={node.cli.datadir}',
                'generate',
                f'--address={node.getnewaddress()}',
                f'--grind-cmd={self.options.freicoinutil} grind',
                '--nbits=1d00ffff',
                f'--set-block-time={int(time.time())}',
            ], check=True, stderr=subprocess.STDOUT)
        assert_equal(node.getblockcount(), 1)


if __name__ == "__main__":
    SignetMinerTest().main()
