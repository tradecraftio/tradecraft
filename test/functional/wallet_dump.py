#!/usr/bin/env python3
# Copyright (c) 2016-2018 The Bitcoin Core developers
# Copyright (c) 2010-2021 The Freicoin Developers
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
"""Test the dumpwallet RPC."""
import os

from test_framework import segwit_addr
from test_framework.script import ripemd160
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


def read_dump(file_name, addrs, script_addrs, witscript_addrs, hd_master_addr_old):
    """
    Read the given dump, count the addrs that match, count change and reserve.
    Also check that the old hd_master is inactive
    """
    with open(file_name, encoding='utf8') as inputfile:
        found_legacy_addr = 0
        found_p2sh_segwit_addr = 0
        found_bech32_addr = 0
        found_script_addr = 0
        found_witscript_addr = 0
        found_addr_chg = 0
        found_addr_rsv = 0
        hd_master_addr_ret = None
        for line in inputfile:
            # only read non comment lines
            if line[0] != "#" and len(line) > 10:
                # split out some data
                key_date_label, comment = line.split("#")
                key_date_label = key_date_label.split(" ")
                # key = key_date_label[0]
                date = key_date_label[1]
                keytype = key_date_label[2]

                imported_key = date == '1970-01-01T00:00:01Z'
                if imported_key:
                    # Imported keys have multiple addresses, no label (keypath) and timestamp
                    # Skip them
                    continue

                addr_keypath = comment.split(" addr=")[1]
                addr = addr_keypath.split(" ")[0]
                keypath = None
                if keytype == "inactivehdseed=1":
                    # ensure the old master is still available
                    assert (hd_master_addr_old == addr)
                elif keytype == "hdseed=1":
                    # ensure we have generated a new hd master key
                    assert (hd_master_addr_old != addr)
                    hd_master_addr_ret = addr
                elif keytype == "script=1" or keytype == "witver=0":
                    # scripts don't have keypaths
                    keypath = None
                else:
                    keypath = addr_keypath.rstrip().split("hdkeypath=")[1]

                # count key types
                for addrObj in addrs:
                    if addrObj['address'] == addr.split(",")[0] and addrObj['hdkeypath'] == keypath and keytype == "label=":
                        if addr.startswith('m') or addr.startswith('n'):
                            # P2PKH address
                            found_legacy_addr += 1
                        elif addr.startswith('2'):
                            # P2SH-segwit address
                            found_p2sh_segwit_addr += 1
                        elif addr.startswith('bcrt1'):
                            found_bech32_addr += 1
                        break
                    elif keytype == "change=1":
                        found_addr_chg += 1
                        break
                    elif keytype == "reserve=1":
                        found_addr_rsv += 1
                        break

                # count scripts
                for script_addr in script_addrs:
                    if script_addr == addr.rstrip() and (keytype == "script=1"):
                        found_script_addr += 1
                        break
                for witscript_addr in witscript_addrs:
                    if witscript_addr == addr.rstrip() and (keytype == "witver=0"):
                        found_witscript_addr += 1
                        break;

        return found_legacy_addr, found_p2sh_segwit_addr, found_bech32_addr, found_script_addr, found_witscript_addr, found_addr_chg, found_addr_rsv, hd_master_addr_ret


class WalletDumpTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-keypool=90", "-addresstype=legacy"]]
        self.rpc_timeout = 120

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

    def run_test(self):
        wallet_unenc_dump = os.path.join(self.nodes[0].datadir, "wallet.unencrypted.dump")
        wallet_enc_dump = os.path.join(self.nodes[0].datadir, "wallet.encrypted.dump")

        # generate 30 addresses to compare against the dump
        # - 10 legacy P2PKH
        # - 10 P2SH-segwit
        # - 10 bech32
        test_addr_count = 10
        addrs = []
        for address_type in ['legacy', 'p2sh-segwit', 'bech32']:
            for i in range(0, test_addr_count):
                addr = self.nodes[0].getnewaddress(address_type=address_type)
                vaddr = self.nodes[0].getaddressinfo(addr)  # required to get hd keypath
                addrs.append(vaddr)

        # Test scripts dump by adding a 1-of-1 multisig address
        multisig_addr = self.nodes[0].addmultisigaddress(1, [addrs[1]["address"]])["address"]
        p2wsh_addr = self.nodes[0].addwitnessaddress(multisig_addr, False)
        p2sh_p2wsh_addr = self.nodes[0].addwitnessaddress(multisig_addr, True)

        # Construct the P2WPK address from our generated P2WSH address
        addr_info = self.nodes[0].getaddressinfo(p2wsh_addr)
        p2wpk_addr = segwit_addr.encode(p2wsh_addr.split('1')[0], addr_info['witness_version'], ripemd160(bytes.fromhex(addr_info['witness_program'])))

        # Refill the keypool. getnewaddress() refills the keypool *before* taking a key from
        # the keypool, so the final call to getnewaddress leaves the keypool with one key below
        # its capacity
        self.nodes[0].keypoolrefill()

        # dump unencrypted wallet
        result = self.nodes[0].dumpwallet(wallet_unenc_dump)
        assert_equal(result['filename'], wallet_unenc_dump)

        found_legacy_addr, found_p2sh_segwit_addr, found_bech32_addr, found_script_addr, found_witscript_addr, found_addr_chg, found_addr_rsv, hd_master_addr_unenc = \
            read_dump(wallet_unenc_dump, addrs, [multisig_addr, p2sh_p2wsh_addr], [p2wpk_addr], None)
        assert_equal(found_legacy_addr, test_addr_count)  # all keys must be in the dump
        assert_equal(found_p2sh_segwit_addr, test_addr_count)  # all keys must be in the dump
        assert_equal(found_bech32_addr, test_addr_count)  # all keys must be in the dump
        assert_equal(found_script_addr, 2)  # all scripts must be in the dump
        assert_equal(found_witscript_addr, 1)  # all witscripts must be in the dump
        assert_equal(found_addr_chg, 0)  # 0 blocks where mined
        assert_equal(found_addr_rsv, 90 * 2)  # 90 keys plus 100% internal keys

        # encrypt wallet, restart, unlock and dump
        self.nodes[0].encryptwallet('test')
        self.nodes[0].walletpassphrase('test', 10)
        # Should be a no-op:
        self.nodes[0].keypoolrefill()
        self.nodes[0].dumpwallet(wallet_enc_dump)

        found_legacy_addr, found_p2sh_segwit_addr, found_bech32_addr, found_script_addr, found_witscript_addr, found_addr_chg, found_addr_rsv, _ = \
            read_dump(wallet_enc_dump, addrs, [multisig_addr, p2sh_p2wsh_addr], [p2wpk_addr], hd_master_addr_unenc)
        assert_equal(found_legacy_addr, test_addr_count)  # all keys must be in the dump
        assert_equal(found_p2sh_segwit_addr, test_addr_count)  # all keys must be in the dump
        assert_equal(found_bech32_addr, test_addr_count)  # all keys must be in the dump
        assert_equal(found_script_addr, 2)
        assert_equal(found_witscript_addr, 1)
        assert_equal(found_addr_chg, 90 * 2)  # old reserve keys are marked as change now
        assert_equal(found_addr_rsv, 90 * 2)

        # Overwriting should fail
        assert_raises_rpc_error(-8, "already exists", lambda: self.nodes[0].dumpwallet(wallet_enc_dump))

        # Restart node with new wallet, and test importwallet
        self.stop_node(0)
        self.start_node(0, ['-wallet=w2'])

        # Make sure the address is not IsMine before import
        result = self.nodes[0].getaddressinfo(multisig_addr)
        assert not result['ismine']

        self.nodes[0].importwallet(wallet_unenc_dump)

        # Now check IsMine is true
        result = self.nodes[0].getaddressinfo(multisig_addr)
        assert result['ismine']

if __name__ == '__main__':
    WalletDumpTest().main()
