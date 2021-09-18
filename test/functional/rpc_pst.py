#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
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
"""Test the Partially Signed Transaction RPCs.
"""
from decimal import Decimal
from itertools import product

from test_framework.descriptors import descsum_create
from test_framework.key import H_POINT
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    MAX_BIP125_RBF_SEQUENCE,
    WITNESS_SCALE_FACTOR,
    ser_compact_size,
)
from test_framework.pst import (
    PST,
    PSTMap,
    PST_GLOBAL_UNSIGNED_TX,
    PST_IN_RIPEMD160,
    PST_IN_SHA256,
    PST_IN_HASH160,
    PST_IN_HASH256,
    PST_IN_NON_WITNESS_UTXO,
    PST_IN_WITNESS_UTXO,
    PST_OUT_TAP_TREE,
)
from test_framework.script import CScript, OP_TRUE
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    find_output,
    find_vout_for_address,
    random_bytes,
)
from test_framework.wallet_util import (
    generate_keypair,
    get_generate_key,
)

import json
import os


class PSTTest(FreicoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            ["-walletrbf=1", "-addresstype=bech32", "-changetype=bech32"], #TODO: Remove address type restrictions once taproot has pst extensions
            ["-walletrbf=0", "-changetype=legacy"],
            []
        ]
        # whitelist peers to speed up tx relay / mempool sync
        for args in self.extra_args:
            args.append("-whitelist=noban@127.0.0.1")
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_utxo_conversion(self):
        self.log.info("Check that non-witness UTXOs are removed for segwit v1+ inputs")
        mining_node = self.nodes[2]
        offline_node = self.nodes[0]
        online_node = self.nodes[1]

        # Disconnect offline node from others
        # Topology of test network is linear, so this one call is enough
        self.disconnect_nodes(0, 1)

        # Create watchonly on online_node
        online_node.createwallet(wallet_name='wonline', disable_private_keys=True)
        wonline = online_node.get_wallet_rpc('wonline')
        w2 = online_node.get_wallet_rpc(self.default_wallet_name)

        # Mine a transaction that credits the offline address
        offline_addr = offline_node.getnewaddress(address_type="bech32m")
        online_addr = w2.getnewaddress(address_type="bech32m")
        wonline.importaddress(offline_addr, "", False)
        mining_wallet = mining_node.get_wallet_rpc(self.default_wallet_name)
        mining_wallet.sendtoaddress(address=offline_addr, amount=1.0)
        self.generate(mining_node, nblocks=1, sync_fun=lambda: self.sync_all([online_node, mining_node]))

        # Construct an unsigned PST on the online node
        utxos = wonline.listunspent(addresses=[offline_addr])
        raw = wonline.createrawtransaction([{"txid":utxos[0]["txid"], "vout":utxos[0]["vout"]}],[{online_addr:0.9999}])
        pst = wonline.walletprocesspst(online_node.converttopst(raw))["pst"]
        assert not "not_witness_utxo" in mining_node.decodepst(pst)["inputs"][0]

        # add non-witness UTXO manually
        pst_new = PST.fromhex(pst)
        prev_tx = wonline.gettransaction(utxos[0]["txid"])["hex"]
        pst_new.i[0].map[PST_IN_NON_WITNESS_UTXO] = bytes.fromhex(prev_tx)
        assert "non_witness_utxo" in mining_node.decodepst(pst_new.hex())["inputs"][0]

        # Have the offline node sign the PST (which will remove the non-witness UTXO)
        signed_pst = offline_node.walletprocesspst(pst_new.hex())
        assert not "non_witness_utxo" in mining_node.decodepst(signed_pst["pst"])["inputs"][0]

        # Make sure we can mine the resulting transaction
        txid = mining_node.sendrawtransaction(signed_pst["hex"])
        self.generate(mining_node, nblocks=1, sync_fun=lambda: self.sync_all([online_node, mining_node]))
        assert_equal(online_node.gettxout(txid,0)["confirmations"], 1)

        wonline.unloadwallet()

        # Reconnect
        self.connect_nodes(1, 0)
        self.connect_nodes(0, 2)

    def test_input_confs_control(self):
        self.nodes[0].createwallet("minconf")
        wallet = self.nodes[0].get_wallet_rpc("minconf")

        # Fund the wallet with different chain heights
        for _ in range(2):
            self.nodes[1].sendmany("", {wallet.getnewaddress():1, wallet.getnewaddress():1})
            self.generate(self.nodes[1], 1)

        unconfirmed_txid = wallet.sendtoaddress(wallet.getnewaddress(), 0.5)

        self.log.info("Crafting PST using an unconfirmed input")
        target_address = self.nodes[1].getnewaddress()
        pstx1 = wallet.walletcreatefundedpst([], {target_address: 0.1}, 0, {'fee_rate': 1, 'maxconf': 0})['pst']

        # Make sure we only had the one input
        tx1_inputs = self.nodes[0].decodepst(pstx1)['tx']['vin']
        assert_equal(len(tx1_inputs), 1)

        utxo1 = tx1_inputs[0]
        assert_equal(unconfirmed_txid, utxo1['txid'])

        signed_tx1 = wallet.walletprocesspst(pstx1)
        txid1 = self.nodes[0].sendrawtransaction(signed_tx1['hex'])

        mempool = self.nodes[0].getrawmempool()
        assert txid1 in mempool

        self.log.info("Fail to craft a new PST that sends more funds with add_inputs = False")
        assert_raises_rpc_error(-4, "The preselected coins total amount does not cover the transaction target. Please allow other inputs to be automatically selected or include more coins manually", wallet.walletcreatefundedpst, [{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1}, 0, {'add_inputs': False})

        self.log.info("Fail to craft a new PST with minconf above highest one")
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.walletcreatefundedpst, [{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1}, 0, {'add_inputs': True, 'minconf': 3, 'fee_rate': 10})

        self.log.info("Fail to broadcast a new PST with maxconf 0 due to BIP125 rules to verify it actually chose unconfirmed outputs")
        pst_invalid = wallet.walletcreatefundedpst([{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1}, 0, {'add_inputs': True, 'maxconf': 0, 'fee_rate': 10})['pst']
        signed_invalid = wallet.walletprocesspst(pst_invalid)
        assert_raises_rpc_error(-26, "bad-txns-spends-conflicting-tx", self.nodes[0].sendrawtransaction, signed_invalid['hex'])

        self.log.info("Craft a replacement adding inputs with highest confs possible")
        pstx2 = wallet.walletcreatefundedpst([{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1}, 0, {'add_inputs': True, 'minconf': 2, 'fee_rate': 10})['pst']
        tx2_inputs = self.nodes[0].decodepst(pstx2)['tx']['vin']
        assert_greater_than_or_equal(len(tx2_inputs), 2)
        for vin in tx2_inputs:
            if vin['txid'] != unconfirmed_txid:
                assert_greater_than_or_equal(self.nodes[0].gettxout(vin['txid'], vin['vout'])['confirmations'], 2)

        signed_tx2 = wallet.walletprocesspst(pstx2)
        txid2 = self.nodes[0].sendrawtransaction(signed_tx2['hex'])

        mempool = self.nodes[0].getrawmempool()
        assert txid1 not in mempool
        assert txid2 in mempool

        wallet.unloadwallet()

    def assert_change_type(self, pstx, expected_type):
        """Assert that the given PST has a change output with the given type."""

        # The decodepst RPC is stateless and independent of any settings, we can always just call it on the first node
        decoded_pst = self.nodes[0].decodepst(pstx["pst"])
        changepos = pstx["changepos"]
        assert_equal(decoded_pst["tx"]["vout"][changepos]["scriptPubKey"]["type"], expected_type)

    def run_test(self):
        # Create and fund a raw tx for sending 10 FRC
        pstx1 = self.nodes[0].walletcreatefundedpst([], {self.nodes[2].getnewaddress():10})['pst']

        # If inputs are specified, do not automatically add more:
        utxo1 = self.nodes[0].listunspent()[0]
        assert_raises_rpc_error(-4, "The preselected coins total amount does not cover the transaction target. "
                                    "Please allow other inputs to be automatically selected or include more coins manually",
                                self.nodes[0].walletcreatefundedpst, [{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():90})

        pstx1 = self.nodes[0].walletcreatefundedpst([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():90}, 0, {"add_inputs": True})['pst']
        assert_equal(len(self.nodes[0].decodepst(pstx1)['tx']['vin']), 2)

        # Inputs argument can be null
        self.nodes[0].walletcreatefundedpst(None, {self.nodes[2].getnewaddress():10})

        # Node 1 should not be able to add anything to it but still return the pstx same as before
        pstx = self.nodes[1].walletprocesspst(pstx1)['pst']
        assert_equal(pstx1, pstx)

        # Node 0 should not be able to sign the transaction with the wallet is locked
        self.nodes[0].encryptwallet("password")
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase with walletpassphrase first", self.nodes[0].walletprocesspst, pstx)

        # Node 0 should be able to process without signing though
        unsigned_tx = self.nodes[0].walletprocesspst(pstx, False)
        assert_equal(unsigned_tx['complete'], False)

        self.nodes[0].walletpassphrase(passphrase="password", timeout=1000000)

        # Sign the transaction but don't finalize
        processed_pst = self.nodes[0].walletprocesspst(pst=pstx, finalize=False)
        assert "hex" not in processed_pst
        signed_pst = processed_pst['pst']

        # Finalize and send
        finalized_hex = self.nodes[0].finalizepst(signed_pst)['hex']
        self.nodes[0].sendrawtransaction(finalized_hex)

        # Alternative method: sign AND finalize in one command
        processed_finalized_pst = self.nodes[0].walletprocesspst(pst=pstx, finalize=True)
        finalized_pst = processed_finalized_pst['pst']
        finalized_pst_hex = processed_finalized_pst['hex']
        assert signed_pst != finalized_pst
        assert finalized_pst_hex == finalized_hex

        # Manually selected inputs can be locked:
        assert_equal(len(self.nodes[0].listlockunspent()), 0)
        utxo1 = self.nodes[0].listunspent()[0]
        pstx1 = self.nodes[0].walletcreatefundedpst([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():1}, 0,{"lockUnspents": True})["pst"]
        assert_equal(len(self.nodes[0].listlockunspent()), 1)

        # Locks are ignored for manually selected inputs
        self.nodes[0].walletcreatefundedpst([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():1}, 0)

        # Create p2sh, p2wpkh, and p2wsh addresses
        pubkey0 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())['pubkey']
        pubkey1 = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())['pubkey']
        pubkey2 = self.nodes[2].getaddressinfo(self.nodes[2].getnewaddress())['pubkey']

        # Setup watchonly wallets
        self.nodes[2].createwallet(wallet_name='wmulti', disable_private_keys=True)
        wmulti = self.nodes[2].get_wallet_rpc('wmulti')

        # Create all the addresses
        p2sh = wmulti.addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "legacy")['address']
        p2wsh = wmulti.addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "bech32")['address']
        p2sh_p2wsh = wmulti.addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "p2sh-segwit")['address']
        if not self.options.descriptors:
            wmulti.importaddress(p2sh)
            wmulti.importaddress(p2wsh)
            wmulti.importaddress(p2sh_p2wsh)
        p2wpkh = self.nodes[1].getnewaddress("", "bech32")
        p2pkh = self.nodes[1].getnewaddress("", "legacy")
        p2sh_p2wpkh = self.nodes[1].getnewaddress("", "p2sh-segwit")

        # fund those addresses
        rawtx = self.nodes[0].createrawtransaction([], {p2sh:10, p2wsh:10, p2wpkh:10, p2sh_p2wsh:10, p2sh_p2wpkh:10, p2pkh:10})
        rawtx = self.nodes[0].fundrawtransaction(rawtx, {"changePosition":3})
        signed_tx = self.nodes[0].signrawtransactionwithwallet(rawtx['hex'])['hex']
        txid = self.nodes[0].sendrawtransaction(signed_tx)
        self.generate(self.nodes[0], 6)

        # Find the output pos
        p2sh_pos = -1
        p2wsh_pos = -1
        p2wpkh_pos = -1
        p2pkh_pos = -1
        p2sh_p2wsh_pos = -1
        p2sh_p2wpkh_pos = -1
        decoded = self.nodes[0].decoderawtransaction(signed_tx)
        for out in decoded['vout']:
            if out['scriptPubKey']['address'] == p2sh:
                p2sh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2wsh:
                p2wsh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2wpkh:
                p2wpkh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2sh_p2wsh:
                p2sh_p2wsh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2sh_p2wpkh:
                p2sh_p2wpkh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2pkh:
                p2pkh_pos = out['n']

        inputs = [{"txid": txid, "vout": p2wpkh_pos}, {"txid": txid, "vout": p2sh_p2wpkh_pos}, {"txid": txid, "vout": p2pkh_pos}]
        outputs = [{self.nodes[1].getnewaddress(): 29.99}]

        # spend single key from node 1
        created_pst = self.nodes[1].walletcreatefundedpst(inputs, outputs)
        walletprocesspst_out = self.nodes[1].walletprocesspst(created_pst['pst'])
        # Make sure it has both types of UTXOs
        decoded = self.nodes[1].decodepst(walletprocesspst_out['pst'])
        assert 'non_witness_utxo' in decoded['inputs'][0]
        assert 'witness_utxo' in decoded['inputs'][0]
        # Check decodepst fee calculation (input values shall only be counted once per UTXO)
        assert_equal(decoded['fee'], created_pst['fee'])
        assert_equal(walletprocesspst_out['complete'], True)
        self.nodes[1].sendrawtransaction(walletprocesspst_out['hex'])

        self.log.info("Test walletcreatefundedpst fee rate of 10000 sat/vB and 0.1 FRC/kvB produces a total fee at or slightly below -maxtxfee (~0.05290000)")
        res1 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {"fee_rate": 10000, "add_inputs": True})
        assert_approx(res1["fee"], 0.055, 0.005)
        res2 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {"feeRate": "0.1", "add_inputs": True})
        assert_approx(res2["fee"], 0.055, 0.005)

        self.log.info("Test min fee rate checks with walletcreatefundedpst are bypassed, e.g. a fee_rate under 1 sat/vB is allowed")
        res3 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {"fee_rate": "0.999", "add_inputs": True})
        assert_approx(res3["fee"], 0.00000381, 0.0000001)
        res4 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {"feeRate": 0.00000999, "add_inputs": True})
        assert_approx(res4["fee"], 0.00000381, 0.0000001)

        self.log.info("Test min fee rate checks with walletcreatefundedpst are bypassed and that funding non-standard 'zero-fee' transactions is valid")
        for param, zero_value in product(["fee_rate", "feeRate"], [0, 0.000, 0.00000000, "0", "0.000", "0.00000000"]):
            assert_equal(0, self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {param: zero_value, "add_inputs": True})["fee"])

        self.log.info("Test invalid fee rate settings")
        for param, value in {("fee_rate", 100000), ("feeRate", 1)}:
            assert_raises_rpc_error(-4, "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {param: value, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount out of range",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {param: -1, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount is not a number or string",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {param: {"foo": "bar"}, "add_inputs": True})
            # Test fee rate values that don't pass fixed-point parsing checks.
            for invalid_value in ["", 0.000000001, 1e-09, 1.111111111, 1111111111111111, "31.999999999999999999999"]:
                assert_raises_rpc_error(-3, "Invalid amount",
                    self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {param: invalid_value, "add_inputs": True})
        # Test fee_rate values that cannot be represented in sat/vB.
        for invalid_value in [0.0001, 0.00000001, 0.00099999, 31.99999999]:
            assert_raises_rpc_error(-3, "Invalid amount",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"fee_rate": invalid_value, "add_inputs": True})

        self.log.info("- raises RPC error if both feeRate and fee_rate are passed")
        assert_raises_rpc_error(-8, "Cannot specify both fee_rate (sat/vB) and feeRate (FRC/kvB)",
            self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"fee_rate": 0.1, "feeRate": 0.1, "add_inputs": True})

        self.log.info("- raises RPC error if both feeRate and estimate_mode passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and feeRate",
            self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"estimate_mode": "economical", "feeRate": 0.1, "add_inputs": True})

        for param in ["feeRate", "fee_rate"]:
            self.log.info("- raises RPC error if both {} and conf_target are passed".format(param))
            assert_raises_rpc_error(-8, "Cannot specify both conf_target and {}. Please provide either a confirmation "
                "target in blocks for automatic fee estimation, or an explicit fee rate.".format(param),
                self.nodes[1].walletcreatefundedpst ,inputs, outputs, 0, {param: 1, "conf_target": 1, "add_inputs": True})

        self.log.info("- raises RPC error if both fee_rate and estimate_mode are passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and fee_rate",
            self.nodes[1].walletcreatefundedpst ,inputs, outputs, 0, {"fee_rate": 1, "estimate_mode": "economical", "add_inputs": True})

        self.log.info("- raises RPC error with invalid estimate_mode settings")
        for k, v in {"number": 42, "object": {"foo": "bar"}}.items():
            assert_raises_rpc_error(-3, f"JSON value of type {k} for field estimate_mode is not of expected type string",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"estimate_mode": v, "conf_target": 0.1, "add_inputs": True})
        for mode in ["", "foo", Decimal("3.141592")]:
            assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"',
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"estimate_mode": mode, "conf_target": 0.1, "add_inputs": True})

        self.log.info("- raises RPC error with invalid conf_target settings")
        for mode in ["unset", "economical", "conservative"]:
            self.log.debug("{}".format(mode))
            for k, v in {"string": "", "object": {"foo": "bar"}}.items():
                assert_raises_rpc_error(-3, f"JSON value of type {k} for field conf_target is not of expected type number",
                    self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"estimate_mode": mode, "conf_target": v, "add_inputs": True})
            for n in [-1, 0, 1009]:
                assert_raises_rpc_error(-8, "Invalid conf_target, must be between 1 and 1008",  # max value of 1008 per src/policy/fees.h
                    self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"estimate_mode": mode, "conf_target": n, "add_inputs": True})

        self.log.info("Test walletcreatefundedpst with too-high fee rate produces total fee well above -maxtxfee and raises RPC error")
        # previously this was silently capped at -maxtxfee
        for bool_add, outputs_array in {True: outputs, False: [{self.nodes[1].getnewaddress(): 1}]}.items():
            msg = "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)"
            assert_raises_rpc_error(-4, msg, self.nodes[1].walletcreatefundedpst, inputs, outputs_array, 0, {"fee_rate": 1000000, "add_inputs": bool_add})
            assert_raises_rpc_error(-4, msg, self.nodes[1].walletcreatefundedpst, inputs, outputs_array, 0, {"feeRate": 1, "add_inputs": bool_add})

        self.log.info("Test various PST operations")
        # partially sign multisig things with node 1
        pstx = wmulti.walletcreatefundedpst(inputs=[{"txid":txid,"vout":p2wsh_pos},{"txid":txid,"vout":p2sh_pos},{"txid":txid,"vout":p2sh_p2wsh_pos}], outputs={self.nodes[1].getnewaddress():29.99}, changeAddress=self.nodes[1].getrawchangeaddress())['pst']
        walletprocesspst_out = self.nodes[1].walletprocesspst(pstx)
        pstx = walletprocesspst_out['pst']
        assert_equal(walletprocesspst_out['complete'], False)

        # Unload wmulti, we don't need it anymore
        wmulti.unloadwallet()

        # partially sign with node 2. This should be complete and sendable
        walletprocesspst_out = self.nodes[2].walletprocesspst(pstx)
        assert_equal(walletprocesspst_out['complete'], True)
        self.nodes[2].sendrawtransaction(walletprocesspst_out['hex'])

        # check that walletprocesspst fails to decode a non-pst
        rawtx = self.nodes[1].createrawtransaction([{"txid":txid,"vout":p2wpkh_pos}], {self.nodes[1].getnewaddress():9.99})
        assert_raises_rpc_error(-22, "TX decode failed", self.nodes[1].walletprocesspst, rawtx)

        # Convert a non-pst to pst and make sure we can decode it
        rawtx = self.nodes[0].createrawtransaction([], {self.nodes[1].getnewaddress():10})
        rawtx = self.nodes[0].fundrawtransaction(rawtx)
        new_pst = self.nodes[0].converttopst(rawtx['hex'])
        self.nodes[0].decodepst(new_pst)

        # Make sure that a non-pst with signatures cannot be converted
        signedtx = self.nodes[0].signrawtransactionwithwallet(rawtx['hex'])
        assert_raises_rpc_error(-22, "Inputs must not have scriptSigs and scriptWitnesses",
                                self.nodes[0].converttopst, hexstring=signedtx['hex'])  # permitsigdata=False by default
        assert_raises_rpc_error(-22, "Inputs must not have scriptSigs and scriptWitnesses",
                                self.nodes[0].converttopst, hexstring=signedtx['hex'], permitsigdata=False)
        assert_raises_rpc_error(-22, "Inputs must not have scriptSigs and scriptWitnesses",
                                self.nodes[0].converttopst, hexstring=signedtx['hex'], permitsigdata=False, iswitness=True)
        # Unless we allow it to convert and strip signatures
        self.nodes[0].converttopst(hexstring=signedtx['hex'], permitsigdata=True)

        # Create outputs to nodes 1 and 2
        node1_addr = self.nodes[1].getnewaddress()
        node2_addr = self.nodes[2].getnewaddress()
        txid1 = self.nodes[0].sendtoaddress(node1_addr, 13)
        txid2 = self.nodes[0].sendtoaddress(node2_addr, 13)
        blockhash = self.generate(self.nodes[0], 6)[0]
        vout1 = find_output(self.nodes[1], txid1, 13, blockhash=blockhash)
        vout2 = find_output(self.nodes[2], txid2, 13, blockhash=blockhash)

        # Create a pst spending outputs from nodes 1 and 2
        pst_orig = self.nodes[0].createpst([{"txid":txid1,  "vout":vout1}, {"txid":txid2, "vout":vout2}], {self.nodes[0].getnewaddress():25.999})

        # Update psts, should only have data for one input and not the other
        pst1 = self.nodes[1].walletprocesspst(pst_orig, False, "ALL")['pst']
        pst1_decoded = self.nodes[0].decodepst(pst1)
        assert pst1_decoded['inputs'][0] and not pst1_decoded['inputs'][1]
        # Check that BIP32 path was added
        assert "bip32_derivs" in pst1_decoded['inputs'][0]
        pst2 = self.nodes[2].walletprocesspst(pst_orig, False, "ALL", False)['pst']
        pst2_decoded = self.nodes[0].decodepst(pst2)
        assert not pst2_decoded['inputs'][0] and pst2_decoded['inputs'][1]
        # Check that BIP32 paths were not added
        assert "bip32_derivs" not in pst2_decoded['inputs'][1]

        # Sign PSTs (workaround issue #18039)
        pst1 = self.nodes[1].walletprocesspst(pst_orig)['pst']
        pst2 = self.nodes[2].walletprocesspst(pst_orig)['pst']

        # Combine, finalize, and send the psts
        combined = self.nodes[0].combinepst([pst1, pst2])
        finalized = self.nodes[0].finalizepst(combined)['hex']
        self.nodes[0].sendrawtransaction(finalized)
        self.generate(self.nodes[0], 6)

        # Test additional args in walletcreatepst
        # Make sure both pre-included and funded inputs
        # have the correct sequence numbers based on
        # replaceable arg
        block_height = self.nodes[0].getblockcount()
        unspent = self.nodes[0].listunspent()[0]
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, {"replaceable": False, "add_inputs": True}, False)
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_greater_than(tx_in["sequence"], MAX_BIP125_RBF_SEQUENCE)
            assert "bip32_derivs" not in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], block_height+2)

        # Same construction with only locktime set and RBF explicitly enabled
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height, {"replaceable": True, "add_inputs": True}, True)
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_BIP125_RBF_SEQUENCE)
            assert "bip32_derivs" in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], block_height)

        # Same construction without optional arguments
        pstx_info = self.nodes[0].walletcreatefundedpst([], [{self.nodes[2].getnewaddress():unspent["amount"]+1}])
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_BIP125_RBF_SEQUENCE)
            assert "bip32_derivs" in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], 0)

        # Same construction without optional arguments, for a node with -walletrbf=0
        unspent1 = self.nodes[1].listunspent()[0]
        pstx_info = self.nodes[1].walletcreatefundedpst([{"txid":unspent1["txid"], "vout":unspent1["vout"]}], [{self.nodes[2].getnewaddress():unspent1["amount"]+1}], block_height, {"add_inputs": True})
        decoded_pst = self.nodes[1].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_greater_than(tx_in["sequence"], MAX_BIP125_RBF_SEQUENCE)
            assert "bip32_derivs" in pst_in

        # Make sure change address wallet does not have P2SH innerscript access to results in success
        # when attempting BnB coin selection
        self.nodes[0].walletcreatefundedpst([], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, {"changeAddress":self.nodes[1].getnewaddress()}, False)

        # Make sure the wallet's change type is respected by default
        small_output = {self.nodes[0].getnewaddress():0.1}
        pstx_native = self.nodes[0].walletcreatefundedpst([], [small_output])
        self.assert_change_type(pstx_native, "witness_v0_keyhash")
        pstx_legacy = self.nodes[1].walletcreatefundedpst([], [small_output])
        self.assert_change_type(pstx_legacy, "pubkeyhash")

        # Make sure the change type of the wallet can also be overwritten
        pstx_np2wkh = self.nodes[1].walletcreatefundedpst([], [small_output], 0, {"change_type":"p2sh-segwit"})
        self.assert_change_type(pstx_np2wkh, "scripthash")

        # Make sure the change type cannot be specified if a change address is given
        invalid_options = {"change_type":"legacy","changeAddress":self.nodes[0].getnewaddress()}
        assert_raises_rpc_error(-8, "both change address and address type options", self.nodes[0].walletcreatefundedpst, [], [small_output], 0, invalid_options)

        # Regression test for 14473 (mishandling of already-signed witness transaction):
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], 0, {"add_inputs": True})
        complete_pst = self.nodes[0].walletprocesspst(pstx_info["pst"])
        double_processed_pst = self.nodes[0].walletprocesspst(complete_pst["pst"])
        assert_equal(complete_pst, double_processed_pst)
        # We don't care about the decode result, but decoding must succeed.
        self.nodes[0].decodepst(double_processed_pst["pst"])

        # Make sure unsafe inputs are included if specified
        self.nodes[2].createwallet(wallet_name="unsafe")
        wunsafe = self.nodes[2].get_wallet_rpc("unsafe")
        self.nodes[0].sendtoaddress(wunsafe.getnewaddress(), 2)
        self.sync_mempools()
        assert_raises_rpc_error(-4, "Insufficient funds", wunsafe.walletcreatefundedpst, [], [{self.nodes[0].getnewaddress(): 1}])
        wunsafe.walletcreatefundedpst([], [{self.nodes[0].getnewaddress(): 1}], 0, {"include_unsafe": True})

        # BIP 174 Test Vectors

        # Check that unknown values are just passed through
        unknown_pst = "70736274ff01003f0200000001ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000ffffffff010000000000000000036a010000000000000a0f0102030405060708090f0102030405060708090a0b0c0d0e0f0000"
        unknown_out = self.nodes[0].walletprocesspst(unknown_pst)['pst']
        assert_equal(unknown_pst, unknown_out)

        # Open the data file
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data/rpc_pst.json'), encoding='utf-8') as f:
            d = json.load(f)
            invalids = d['invalid']
            invalid_with_msgs = d["invalid_with_msg"]
            valids = d['valid']
            creators = d['creator']
            signers = d['signer']
            combiners = d['combiner']
            finalizers = d['finalizer']
            extractors = d['extractor']

        # Invalid PSTs
        for invalid in invalids:
            assert_raises_rpc_error(-22, "TX decode failed", self.nodes[0].decodepst, invalid)
        for invalid in invalid_with_msgs:
            pst, msg = invalid
            assert_raises_rpc_error(-22, f"TX decode failed {msg}", self.nodes[0].decodepst, pst)

        # Valid PSTs
        for valid in valids:
            self.nodes[0].decodepst(valid)

        # Creator Tests
        for creator in creators:
            created_tx = self.nodes[0].createpst(inputs=creator['inputs'], outputs=creator['outputs'], replaceable=False)
            assert_equal(created_tx, creator['result'])

        # Signer tests
        for i, signer in enumerate(signers):
            self.nodes[2].createwallet(wallet_name="wallet{}".format(i))
            wrpc = self.nodes[2].get_wallet_rpc("wallet{}".format(i))
            for key in signer['privkeys']:
                wrpc.importprivkey(key)
            signed_tx = wrpc.walletprocesspst(signer['pst'], True, "ALL")['pst']
            assert_equal(signed_tx, signer['result'])

        # Combiner test
        for combiner in combiners:
            combined = self.nodes[2].combinepst(combiner['combine'])
            assert_equal(combined, combiner['result'])

        # Empty combiner test
        assert_raises_rpc_error(-8, "Parameter 'txs' cannot be empty", self.nodes[0].combinepst, [])

        # Finalizer test
        for finalizer in finalizers:
            finalized = self.nodes[2].finalizepst(finalizer['finalize'], False)['pst']
            assert_equal(finalized, finalizer['result'])

        # Extractor test
        for extractor in extractors:
            extracted = self.nodes[2].finalizepst(extractor['extract'], True)['hex']
            assert_equal(extracted, extractor['result'])

        # Unload extra wallets
        for i, signer in enumerate(signers):
            self.nodes[2].unloadwallet("wallet{}".format(i))

        if self.options.descriptors:
            self.test_utxo_conversion()

        self.test_input_confs_control()

        # Test that psts with p2pkh outputs are created properly
        p2pkh = self.nodes[0].getnewaddress(address_type='legacy')
        pst = self.nodes[1].walletcreatefundedpst([], [{p2pkh : 1}], 0, {"includeWatching" : True}, True)
        self.nodes[0].decodepst(pst['pst'])

        # Test decoding error: invalid hex
        assert_raises_rpc_error(-22, "TX decode failed invalid hex", self.nodes[0].decodepst, ";definitely not hex;")

        # Send to all types of addresses
        addr1 = self.nodes[1].getnewaddress("", "bech32")
        txid1 = self.nodes[0].sendtoaddress(addr1, 11)
        vout1 = find_output(self.nodes[0], txid1, 11)
        addr2 = self.nodes[1].getnewaddress("", "legacy")
        txid2 = self.nodes[0].sendtoaddress(addr2, 11)
        vout2 = find_output(self.nodes[0], txid2, 11)
        addr3 = self.nodes[1].getnewaddress("", "p2sh-segwit")
        txid3 = self.nodes[0].sendtoaddress(addr3, 11)
        vout3 = find_output(self.nodes[0], txid3, 11)
        self.sync_all()

        def test_pst_input_keys(pst_input, keys):
            """Check that the pst input has only the expected keys."""
            assert_equal(set(keys), set(pst_input.keys()))

        # Create a PST. None of the inputs are filled initially
        pst = self.nodes[1].createpst([{"txid":txid1, "vout":vout1},{"txid":txid2, "vout":vout2},{"txid":txid3, "vout":vout3}], {self.nodes[0].getnewaddress():32.999})
        decoded = self.nodes[1].decodepst(pst)
        test_pst_input_keys(decoded['inputs'][0], [])
        test_pst_input_keys(decoded['inputs'][1], [])
        test_pst_input_keys(decoded['inputs'][2], [])

        # Update a PST with UTXOs from the node
        # Bech32 inputs should be filled with witness UTXO. Other inputs should not be filled because they are non-witness
        updated = self.nodes[1].utxoupdatepst(pst)
        decoded = self.nodes[1].decodepst(updated)
        test_pst_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo'])
        test_pst_input_keys(decoded['inputs'][1], ['non_witness_utxo'])
        test_pst_input_keys(decoded['inputs'][2], ['non_witness_utxo'])

        # Try again, now while providing descriptors, making P2SH-segwit work, and causing bip32_derivs and redeem_script to be filled in
        descs = [self.nodes[1].getaddressinfo(addr)['desc'] for addr in [addr1,addr2,addr3]]
        updated = self.nodes[1].utxoupdatepst(pst=pst, descriptors=descs)
        decoded = self.nodes[1].decodepst(updated)
        test_pst_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo', 'bip32_derivs'])
        test_pst_input_keys(decoded['inputs'][1], ['non_witness_utxo', 'bip32_derivs'])
        test_pst_input_keys(decoded['inputs'][2], ['non_witness_utxo','witness_utxo', 'bip32_derivs', 'redeem_script'])

        # Two PSTs with a common input should not be joinable
        pst1 = self.nodes[1].createpst([{"txid":txid1, "vout":vout1}], {self.nodes[0].getnewaddress():Decimal('10.999')})
        assert_raises_rpc_error(-8, "exists in multiple PSTs", self.nodes[1].joinpsts, [pst1, updated])

        # Join two distinct PSTs
        addr4 = self.nodes[1].getnewaddress("", "p2sh-segwit")
        txid4 = self.nodes[0].sendtoaddress(addr4, 5)
        vout4 = find_output(self.nodes[0], txid4, 5)
        self.generate(self.nodes[0], 6)
        pst2 = self.nodes[1].createpst([{"txid":txid4, "vout":vout4}], {self.nodes[0].getnewaddress():Decimal('4.999')})
        pst2 = self.nodes[1].walletprocesspst(pst2)['pst']
        pst2_decoded = self.nodes[0].decodepst(pst2)
        assert "final_scriptwitness" in pst2_decoded['inputs'][0] and "final_scriptSig" in pst2_decoded['inputs'][0]
        joined = self.nodes[0].joinpsts([pst, pst2])
        joined_decoded = self.nodes[0].decodepst(joined)
        assert len(joined_decoded['inputs']) == 4 and len(joined_decoded['outputs']) == 2 and "final_scriptwitness" not in joined_decoded['inputs'][3] and "final_scriptSig" not in joined_decoded['inputs'][3]

        # Check that joining shuffles the inputs and outputs
        # 10 attempts should be enough to get a shuffled join
        shuffled = False
        for _ in range(10):
            shuffled_joined = self.nodes[0].joinpsts([pst, pst2])
            shuffled |= joined != shuffled_joined
            if shuffled:
                break
        assert shuffled

        # Newly created PST needs UTXOs and updating
        addr = self.nodes[1].getnewaddress("", "p2sh-segwit")
        txid = self.nodes[0].sendtoaddress(addr, 7)
        addrinfo = self.nodes[1].getaddressinfo(addr)
        blockhash = self.generate(self.nodes[0], 6)[0]
        vout = find_output(self.nodes[0], txid, 7, blockhash=blockhash)
        pst = self.nodes[1].createpst([{"txid":txid, "vout":vout}], {self.nodes[0].getnewaddress("", "p2sh-segwit"):Decimal('6.999')})
        analyzed = self.nodes[0].analyzepst(pst)
        assert not analyzed['inputs'][0]['has_utxo'] and not analyzed['inputs'][0]['is_final'] and analyzed['inputs'][0]['next'] == 'updater' and analyzed['next'] == 'updater'

        # After update with wallet, only needs signing
        updated = self.nodes[1].walletprocesspst(pst, False, 'ALL', True)['pst']
        analyzed = self.nodes[0].analyzepst(updated)
        assert analyzed['inputs'][0]['has_utxo'] and not analyzed['inputs'][0]['is_final'] and analyzed['inputs'][0]['next'] == 'signer' and analyzed['next'] == 'signer' and analyzed['inputs'][0]['missing']['signatures'][0] == addrinfo['embedded']['witness_program']

        # Check fee and size things
        assert analyzed['fee'] == Decimal('0.001') and analyzed['estimated_vsize'] == 134 and analyzed['estimated_feerate'] == Decimal('0.00746268')

        # After signing and finalizing, needs extracting
        signed = self.nodes[1].walletprocesspst(updated)['pst']
        analyzed = self.nodes[0].analyzepst(signed)
        assert analyzed['inputs'][0]['has_utxo'] and analyzed['inputs'][0]['is_final'] and analyzed['next'] == 'extractor'

        self.log.info("PST spending unspendable outputs should have error message and Creator as next")
        analysis = self.nodes[0].analyzepst('70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f545887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160041d85c2b71d0060b09c9886aeb815e50991dda124d00e1f5050000000016ff3fc000d760fffbffffffffff09c4e8742d1d87008f000000000001012000c2eb0b00000000176a14b7f5faf442e3797da74e87fe7d9d7497e3b20289030107090301071000010000800001012000c2eb0b00000000176a14b7f5faf442e3797da74e87fe7d9d7497e3b202890301070903010710d90c6a4f00000000')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 spends unspendable output')

        self.log.info("PST with invalid values should have error message and Creator as next")
        analysis = self.nodes[0].analyzepst('70736274ff0100710200000001f034d01160026eada7ce91a30c07e8130899034be735d47134fded7a0b1b3d010000000000ffffffff0200f902950000000016001428dc34c7c1d172d0209afa1ebe6e2ed526cded72fced029500000000160014f724e2017b88be0d2f8c2d7d100a8106e08624d4000000000001011f0080816ae3d007001600149503b717f63c7a3ae351c438138022f14c353af6000000')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 has invalid value')

        self.log.info("PST with signed, but not finalized, inputs should have Finalizer as next")
        analysis = self.nodes[0].analyzepst('70736274ff0100710200000001961ecdcc5d9db5e84029ab0fbf6dfcbcc382d528f4446a8b193cc208a80286eb0000000000fdffffff0250c3000000000000160014cbf531c59bb366cc1c9859cdfc4f431928872d4b2e18f50500000000160014bb07e68d11dfcc17c7e5f6702a2c4cc0999d0e1b680000000001011f00e1f50500000000160014e048da1e6d85fdfe2df95bd8b182b15499c2c96d220202fe2e1db550110915ad44e1b41c7a0671a564973254730a398689a01a448d027247304402206c7bcca70c206afb704112f4147829db7fa733d32b77ca59c1b20895dbecc56702200b38182a94c1f819b0b51c387f1e6ca598ee787f2132e2529d7bbfc14ff3d6c70101030401000000220602fe2e1db550110915ad44e1b41c7a0671a564973254730a398689a01a448d0272180f05694354000080010000800000008000000000000000000000220202fe088881e7d3520acb0e2511a231840ad5c85a86001a689a485c4e350b28abfb180f056943540000800100008000000080010000000000000000')
        assert_equal(analysis['next'], 'finalizer')

        analysis = self.nodes[0].analyzepst('70736274ff0100710200000001f034d01160026eada7ce91a30c07e8130899034be735d47134fded7a0b1b3d010000000000ffffffff020080816ae3d0070016001428dc34c7c1d172d0209afa1ebe6e2ed526cded72fced029500000000160014f724e2017b88be0d2f8c2d7d100a8106e08624d4000000000001011f00f2052a010000001600149503b717f63c7a3ae351c438138022f14c353af6000000')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Output amount invalid')

        analysis = self.nodes[0].analyzepst('70736274ff01009a02000000024bc45bc3670ed74db43a6c9b37be1edd8b1f7e4825c2afd348ca02552cdb546b0300000000ffffffff4bc45bc3670ed74db43a6c9b37be1edd8b1f7e4825c2afd348ca02552cdb546b0100000000feffffff02c0b60295000000001600148d24ace36946f7b8ec62c6cbe1d46184ddf5bbcd00f902950000000016001428dc34c7c1d172d0209afa1ebe6e2ed526cded72000000000001009d020000000273331adf6d6d547b8de062962919dcc9c236d9a5f7b9783048550a336a2b8d1b0100000000feffffff73331adf6d6d547b8de062962919dcc9c236d9a5f7b9783048550a336a2b8d1b0000000000feffffff0200f90295000000001976a914fdcd751503c2ece6256aca72c77b9fef49ed57c988ac40cd0295000000001600146659051c8d28e0850f3fb87e019ffce46a4c0bb4000000000001011f40cd0295000000001600146659051c8d28e0850f3fb87e019ffce46a4c0bb4000000')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 specifies invalid prevout')

        assert_raises_rpc_error(-25, 'Inputs missing or spent', self.nodes[0].walletprocesspst, '70736274ff01009a02000000024bc45bc3670ed74db43a6c9b37be1edd8b1f7e4825c2afd348ca02552cdb546b0300000000ffffffff4bc45bc3670ed74db43a6c9b37be1edd8b1f7e4825c2afd348ca02552cdb546b0100000000feffffff02c0b60295000000001600148d24ace36946f7b8ec62c6cbe1d46184ddf5bbcd00f902950000000016001428dc34c7c1d172d0209afa1ebe6e2ed526cded72000000000001009d020000000273331adf6d6d547b8de062962919dcc9c236d9a5f7b9783048550a336a2b8d1b0100000000feffffff73331adf6d6d547b8de062962919dcc9c236d9a5f7b9783048550a336a2b8d1b0000000000feffffff0200f90295000000001976a914fdcd751503c2ece6256aca72c77b9fef49ed57c988ac40cd0295000000001600146659051c8d28e0850f3fb87e019ffce46a4c0bb4000000000001011f40cd0295000000001600146659051c8d28e0850f3fb87e019ffce46a4c0bb4000000')

        self.log.info("Test that we can fund psts with external inputs specified")

        privkey, _ = generate_keypair(wif=True)

        self.nodes[1].createwallet("extfund")
        wallet = self.nodes[1].get_wallet_rpc("extfund")

        # Make a weird but signable script. sh(wsh(pkh())) descriptor accomplishes this
        desc = descsum_create("sh(wsh(pkh({})))".format(privkey))
        if self.options.descriptors:
            res = self.nodes[0].importdescriptors([{"desc": desc, "timestamp": "now"}])
        else:
            res = self.nodes[0].importmulti([{"desc": desc, "timestamp": "now"}])
        assert res[0]["success"]
        addr = self.nodes[0].deriveaddresses(desc)[0]
        addr_info = self.nodes[0].getaddressinfo(addr)

        self.nodes[0].sendtoaddress(addr, 10)
        self.nodes[0].sendtoaddress(wallet.getnewaddress(), 10)
        self.generate(self.nodes[0], 6)
        ext_utxo = self.nodes[0].listunspent(addresses=[addr])[0]

        # An external input without solving data should result in an error
        assert_raises_rpc_error(-4, "Not solvable pre-selected input COutPoint(%s, %s)" % (ext_utxo["txid"][0:10], ext_utxo["vout"]), wallet.walletcreatefundedpst, [ext_utxo], {self.nodes[0].getnewaddress(): 15})

        # But funding should work when the solving data is provided
        pst = wallet.walletcreatefundedpst([ext_utxo], {self.nodes[0].getnewaddress(): 15}, 0, {"add_inputs": True, "solving_data": {"pubkeys": [addr_info['pubkey']], "scripts": [addr_info["embedded"]["scriptPubKey"], addr_info["embedded"]["embedded"]["scriptPubKey"]]}})
        signed = wallet.walletprocesspst(pst['pst'])
        assert not signed['complete']
        signed = self.nodes[0].walletprocesspst(signed['pst'])
        assert signed['complete']

        pst = wallet.walletcreatefundedpst([ext_utxo], {self.nodes[0].getnewaddress(): 15}, 0, {"add_inputs": True, "solving_data":{"descriptors": [desc]}})
        signed = wallet.walletprocesspst(pst['pst'])
        assert not signed['complete']
        signed = self.nodes[0].walletprocesspst(signed['pst'])
        assert signed['complete']
        final = signed['hex']

        dec = self.nodes[0].decodepst(signed["pst"])
        for i, txin in enumerate(dec["tx"]["vin"]):
            if txin["txid"] == ext_utxo["txid"] and txin["vout"] == ext_utxo["vout"]:
                input_idx = i
                break
        pst_in = dec["inputs"][input_idx]
        # Calculate the input weight
        # (prevout + sequence + length of scriptSig + scriptsig + 1 byte buffer) * WITNESS_SCALE_FACTOR + num scriptWitness stack items + (length of stack item + stack item) * N stack items + 1 byte buffer
        len_scriptsig = len(pst_in["final_scriptSig"]["hex"]) // 2 if "final_scriptSig" in pst_in else 0
        len_scriptsig += len(ser_compact_size(len_scriptsig)) + 1
        len_scriptwitness = (sum([(len(x) // 2) + len(ser_compact_size(len(x) // 2)) for x in pst_in["final_scriptwitness"]]) + len(pst_in["final_scriptwitness"]) + 1) if "final_scriptwitness" in pst_in else 0
        input_weight = ((40 + len_scriptsig) * WITNESS_SCALE_FACTOR) + len_scriptwitness
        low_input_weight = input_weight // 2
        high_input_weight = input_weight * 2

        # Input weight error conditions
        assert_raises_rpc_error(
            -8,
            "Input weights should be specified in inputs rather than in options.",
            wallet.walletcreatefundedpst,
            inputs=[ext_utxo],
            outputs={self.nodes[0].getnewaddress(): 15},
            options={"input_weights": [{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": 1000}]}
        )

        # Funding should also work if the input weight is provided
        pst = wallet.walletcreatefundedpst(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True,
        )
        signed = wallet.walletprocesspst(pst["pst"])
        signed = self.nodes[0].walletprocesspst(signed["pst"])
        final = signed["hex"]
        assert self.nodes[0].testmempoolaccept([final])[0]["allowed"]
        # Reducing the weight should have a lower fee
        pst2 = wallet.walletcreatefundedpst(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": low_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True,
        )
        assert_greater_than(pst["fee"], pst2["fee"])
        # Increasing the weight should have a higher fee
        pst2 = wallet.walletcreatefundedpst(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True,
        )
        assert_greater_than(pst2["fee"], pst["fee"])
        # The provided weight should override the calculated weight when solving data is provided
        pst3 = wallet.walletcreatefundedpst(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True, solving_data={"descriptors": [desc]},
        )
        assert_equal(pst2["fee"], pst3["fee"])

        # Import the external utxo descriptor so that we can sign for it from the test wallet
        if self.options.descriptors:
            res = wallet.importdescriptors([{"desc": desc, "timestamp": "now"}])
        else:
            res = wallet.importmulti([{"desc": desc, "timestamp": "now"}])
        assert res[0]["success"]
        # The provided weight should override the calculated weight for a wallet input
        pst3 = wallet.walletcreatefundedpst(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True,
        )
        assert_equal(pst2["fee"], pst3["fee"])

        self.log.info("Test signing inputs that the wallet has keys for but is not watching the scripts")
        self.nodes[1].createwallet(wallet_name="scriptwatchonly", disable_private_keys=True)
        watchonly = self.nodes[1].get_wallet_rpc("scriptwatchonly")

        privkey, pubkey = generate_keypair(wif=True)

        desc = descsum_create("wsh(pkh({}))".format(pubkey.hex()))
        if self.options.descriptors:
            res = watchonly.importdescriptors([{"desc": desc, "timestamp": "now"}])
        else:
            res = watchonly.importmulti([{"desc": desc, "timestamp": "now"}])
        assert res[0]["success"]
        addr = self.nodes[0].deriveaddresses(desc)[0]
        self.nodes[0].sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)
        self.nodes[0].importprivkey(privkey)

        pst = watchonly.sendall([wallet.getnewaddress()])["pst"]
        signed_tx = self.nodes[0].walletprocesspst(pst)
        self.nodes[0].sendrawtransaction(signed_tx["hex"])

        # Same test but for taproot
        if self.options.descriptors:
            privkey, pubkey = generate_keypair(wif=True)

            desc = descsum_create("tr({},pk({}))".format(H_POINT, pubkey.hex()))
            res = watchonly.importdescriptors([{"desc": desc, "timestamp": "now"}])
            assert res[0]["success"]
            addr = self.nodes[0].deriveaddresses(desc)[0]
            self.nodes[0].sendtoaddress(addr, 10)
            self.generate(self.nodes[0], 1)
            self.nodes[0].importdescriptors([{"desc": descsum_create("tr({})".format(privkey)), "timestamp":"now"}])

            pst = watchonly.sendall([wallet.getnewaddress(), addr])["pst"]
            processed_pst = self.nodes[0].walletprocesspst(pst)
            txid = self.nodes[0].sendrawtransaction(processed_pst["hex"])
            vout = find_vout_for_address(self.nodes[0], txid, addr)

            # Make sure tap tree is in pst
            parsed_pst = PST.fromhex(pst)
            assert_greater_than(len(parsed_pst.o[vout].map[PST_OUT_TAP_TREE]), 0)
            assert "taproot_tree" in self.nodes[0].decodepst(pst)["outputs"][vout]
            parsed_pst.make_blank()
            comb_pst = self.nodes[0].combinepst([pst, parsed_pst.hex()])
            assert_equal(comb_pst, pst)

            self.log.info("Test that walletprocesspst both updates and signs a non-updated pst containing Taproot inputs")
            addr = self.nodes[0].getnewaddress("", "bech32m")
            txid = self.nodes[0].sendtoaddress(addr, 1)
            vout = find_vout_for_address(self.nodes[0], txid, addr)
            pst = self.nodes[0].createpst([{"txid": txid, "vout": vout}], [{self.nodes[0].getnewaddress(): 0.9999}])
            signed = self.nodes[0].walletprocesspst(pst)
            rawtx = signed["hex"]
            self.nodes[0].sendrawtransaction(rawtx)
            self.generate(self.nodes[0], 1)

            # Make sure tap tree is not in pst
            parsed_pst = PST.fromhex(pst)
            assert PST_OUT_TAP_TREE not in parsed_pst.o[0].map
            assert "taproot_tree" not in self.nodes[0].decodepst(pst)["outputs"][0]
            parsed_pst.make_blank()
            comb_pst = self.nodes[0].combinepst([pst, parsed_pst.hex()])
            assert_equal(comb_pst, pst)

        self.log.info("Test walletprocesspst raises if an invalid sighashtype is passed")
        assert_raises_rpc_error(-8, "all is not a valid sighash parameter.", self.nodes[0].walletprocesspst, pst, sighashtype="all")

        self.log.info("Test decoding PST with per-input preimage types")
        # note that the decodepst RPC doesn't check whether preimages and hashes match
        hash_ripemd160, preimage_ripemd160 = random_bytes(20), random_bytes(50)
        hash_sha256, preimage_sha256 = random_bytes(32), random_bytes(50)
        hash_hash160, preimage_hash160 = random_bytes(20), random_bytes(50)
        hash_hash256, preimage_hash256 = random_bytes(32), random_bytes(50)

        tx = CTransaction()
        tx.vin = [CTxIn(outpoint=COutPoint(hash=int('aa' * 32, 16), n=0), scriptSig=b""),
                  CTxIn(outpoint=COutPoint(hash=int('bb' * 32, 16), n=0), scriptSig=b""),
                  CTxIn(outpoint=COutPoint(hash=int('cc' * 32, 16), n=0), scriptSig=b""),
                  CTxIn(outpoint=COutPoint(hash=int('dd' * 32, 16), n=0), scriptSig=b"")]
        tx.vout = [CTxOut(nValue=0, scriptPubKey=b"")]
        pst = PST()
        pst.g = PSTMap({PST_GLOBAL_UNSIGNED_TX: tx.serialize()})
        pst.i = [PSTMap({bytes([PST_IN_RIPEMD160]) + hash_ripemd160: preimage_ripemd160}),
                  PSTMap({bytes([PST_IN_SHA256]) + hash_sha256: preimage_sha256}),
                  PSTMap({bytes([PST_IN_HASH160]) + hash_hash160: preimage_hash160}),
                  PSTMap({bytes([PST_IN_HASH256]) + hash_hash256: preimage_hash256})]
        pst.o = [PSTMap()]
        res_inputs = self.nodes[0].decodepst(pst.hex())["inputs"]
        assert_equal(len(res_inputs), 4)
        preimage_keys = ["ripemd160_preimages", "sha256_preimages", "hash160_preimages", "hash256_preimages"]
        expected_hashes = [hash_ripemd160, hash_sha256, hash_hash160, hash_hash256]
        expected_preimages = [preimage_ripemd160, preimage_sha256, preimage_hash160, preimage_hash256]
        for res_input, preimage_key, hash, preimage in zip(res_inputs, preimage_keys, expected_hashes, expected_preimages):
            assert preimage_key in res_input
            assert_equal(len(res_input[preimage_key]), 1)
            assert hash.hex() in res_input[preimage_key]
            assert_equal(res_input[preimage_key][hash.hex()], preimage.hex())

        self.log.info("Test that combining PSTs with different transactions fails")
        tx = CTransaction()
        tx.vin = [CTxIn(outpoint=COutPoint(hash=int('aa' * 32, 16), n=0), scriptSig=b"")]
        tx.vout = [CTxOut(nValue=0, scriptPubKey=b"")]
        pst1 = PST(g=PSTMap({PST_GLOBAL_UNSIGNED_TX: tx.serialize()}), i=[PSTMap()], o=[PSTMap()]).hex()
        tx.vout[0].nValue += 1  # slightly modify tx
        pst2 = PST(g=PSTMap({PST_GLOBAL_UNSIGNED_TX: tx.serialize()}), i=[PSTMap()], o=[PSTMap()]).hex()
        assert_raises_rpc_error(-8, "PSTs not compatible (different transactions)", self.nodes[0].combinepst, [pst1, pst2])
        assert_equal(self.nodes[0].combinepst([pst1, pst1]), pst1)

        self.log.info("Test that PST inputs are being checked via script execution")
        acs_prevout = CTxOut(nValue=0, scriptPubKey=CScript([OP_TRUE]))
        tx = CTransaction()
        tx.vin = [CTxIn(outpoint=COutPoint(hash=int('dd' * 32, 16), n=0), scriptSig=b"")]
        tx.vout = [CTxOut(nValue=0, scriptPubKey=b"")]
        pst = PST()
        pst.g = PSTMap({PST_GLOBAL_UNSIGNED_TX: tx.serialize()})
        pst.i = [PSTMap({bytes([PST_IN_WITNESS_UTXO]) : acs_prevout.serialize()})]
        pst.o = [PSTMap()]
        assert_equal(self.nodes[0].finalizepst(pst.hex()),
            {'hex': '0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd0000000000000000000100000000000000000000000000', 'complete': True})

        self.log.info("Test we don't crash when making a 0-value funded transaction at 0 fee without forcing an input selection")
        assert_raises_rpc_error(-4, "Transaction requires one destination of non-0 value, a non-0 feerate, or a pre-selected input", self.nodes[0].walletcreatefundedpst, [], [{"data": "deadbeef"}], 0, {"fee_rate": "0"})

        self.log.info("Test descriptorprocesspst updates and signs a pst with descriptors")

        self.generate(self.nodes[2], 1)

        # Disable the wallet for node 2 since `descriptorprocesspst` does not use the wallet
        self.restart_node(2, extra_args=["-disablewallet"])
        self.connect_nodes(0, 2)
        self.connect_nodes(1, 2)

        key_info = get_generate_key()
        key = key_info.privkey
        address = key_info.p2wpkh_addr

        descriptor = descsum_create(f"wpkh({key})")

        txid = self.nodes[0].sendtoaddress(address, 1)
        self.sync_all()
        vout = find_output(self.nodes[0], txid, 1)

        pst = self.nodes[2].createpst([{"txid": txid, "vout": vout}], {self.nodes[0].getnewaddress(): 0.99999})
        decoded = self.nodes[2].decodepst(pst)
        test_pst_input_keys(decoded['inputs'][0], [])

        # Test that even if the wrong descriptor is given, `witness_utxo` and `non_witness_utxo`
        # are still added to the pst
        alt_descriptor = descsum_create(f"wpkh({get_generate_key().privkey})")
        alt_pst = self.nodes[2].descriptorprocesspst(pst=pst, descriptors=[alt_descriptor], sighashtype="ALL")["pst"]
        decoded = self.nodes[2].decodepst(alt_pst)
        test_pst_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo'])

        # Test that the pst is not finalized and does not have bip32_derivs unless specified
        processed_pst = self.nodes[2].descriptorprocesspst(pst=pst, descriptors=[descriptor], sighashtype="ALL", bip32derivs=True, finalize=False)
        decoded = self.nodes[2].decodepst(processed_pst['pst'])
        test_pst_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo', 'partial_signatures', 'bip32_derivs'])

        # If pst not finalized, test that result does not have hex
        assert "hex" not in processed_pst

        processed_pst = self.nodes[2].descriptorprocesspst(pst=pst, descriptors=[descriptor], sighashtype="ALL", bip32derivs=False, finalize=True)
        decoded = self.nodes[2].decodepst(processed_pst['pst'])
        test_pst_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo', 'final_scriptwitness'])

        # Test pst is complete
        assert_equal(processed_pst['complete'], True)

        # Broadcast transaction
        self.nodes[2].sendrawtransaction(processed_pst['hex'])

        self.log.info("Test descriptorprocesspst raises if an invalid sighashtype is passed")
        assert_raises_rpc_error(-8, "all is not a valid sighash parameter.", self.nodes[2].descriptorprocesspst, pst, [descriptor], sighashtype="all")


if __name__ == '__main__':
    PSTTest().main()
