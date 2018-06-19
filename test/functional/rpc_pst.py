#!/usr/bin/env python3
# Copyright (c) 2018-2021 The Bitcoin Core developers
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
from test_framework.key import ECKey
from test_framework.messages import (
    MAX_SEQUENCE_NONFINAL,
    ser_compact_size,
    WITNESS_SCALE_FACTOR,
)
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    find_output,
)
from test_framework.wallet_util import bytes_to_wif

import json
import os

# Create one-input, one-output, no-fee transaction:
class PSTTest(FreicoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            ["-addresstype=bech32", "-changetype=bech32"], #TODO: Remove address type restrictions once taproot has pst extensions
            ["-changetype=legacy"],
            []
        ]
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    # TODO: Re-enable this test with segwit v1
    def test_utxo_conversion(self):
        mining_node = self.nodes[2]
        offline_node = self.nodes[0]
        online_node = self.nodes[1]

        # Disconnect offline node from others
        # Topology of test network is linear, so this one call is enough
        self.disconnect_nodes(0, 1)

        # Create watchonly on online_node
        online_node.createwallet(wallet_name='wonline', disable_private_keys=True)
        wonline = online_node.get_wallet_rpc('wonline')
        w2 = online_node.get_wallet_rpc('')

        # Mine a transaction that credits the offline address
        offline_addr = offline_node.getnewaddress(address_type="p2sh-segwit")
        online_addr = w2.getnewaddress(address_type="p2sh-segwit")
        wonline.importaddress(offline_addr, "", False)
        mining_node.sendtoaddress(address=offline_addr, amount=1.0)
        self.generate(mining_node, nblocks=1)

        # Construct an unsigned PST on the online node (who doesn't know the output is Segwit, so will include a non-witness UTXO)
        utxos = wonline.listunspent(addresses=[offline_addr])
        raw = wonline.createrawtransaction([{"txid":utxos[0]["txid"], "vout":utxos[0]["vout"]}],[{online_addr:0.9999}])
        pst = wonline.walletprocesspst(online_node.converttopst(raw))["pst"]
        assert "non_witness_utxo" in mining_node.decodepst(pst)["inputs"][0]

        # Have the offline node sign the PST (which will update the UTXO to segwit)
        signed_pst = offline_node.walletprocesspst(pst)["pst"]
        assert "witness_utxo" in mining_node.decodepst(signed_pst)["inputs"][0]

        # Make sure we can mine the resulting transaction
        txid = mining_node.sendrawtransaction(mining_node.finalizepst(signed_pst)["hex"])
        self.generate(mining_node, 1)
        assert_equal(online_node.gettxout(txid,0)["confirmations"], 1)

        wonline.unloadwallet()

        # Reconnect
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)

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
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[0].walletcreatefundedpst, [{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():90})

        pstx1 = self.nodes[0].walletcreatefundedpst([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():90}, 0, utxo1['refheight'], {"add_inputs": True})['pst']
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

        # Sign the transaction and send
        signed_tx = self.nodes[0].walletprocesspst(pst=pstx, finalize=False)['pst']
        finalized_tx = self.nodes[0].walletprocesspst(pst=pstx, finalize=True)['pst']
        assert signed_tx != finalized_tx
        final_tx = self.nodes[0].finalizepst(signed_tx)['hex']
        self.nodes[0].sendrawtransaction(final_tx)

        # Manually selected inputs can be locked:
        assert_equal(len(self.nodes[0].listlockunspent()), 0)
        utxo1 = self.nodes[0].listunspent()[0]
        pstx1 = self.nodes[0].walletcreatefundedpst([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():1}, 0, utxo1['refheight'], {"lockUnspents": True})["pst"]
        assert_equal(len(self.nodes[0].listlockunspent()), 1)

        # Locks are ignored for manually selected inputs
        self.nodes[0].walletcreatefundedpst([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():1}, 0, utxo1['refheight'])

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
        refheight = decoded['lockheight']

        # spend single key from node 1
        created_pst = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, refheight)
        walletprocesspst_out = self.nodes[1].walletprocesspst(created_pst['pst'])
        # Make sure it has both types of UTXOs
        decoded = self.nodes[1].decodepst(walletprocesspst_out['pst'])
        assert 'non_witness_utxo' in decoded['inputs'][0]
        assert 'witness_utxo' in decoded['inputs'][0]
        # Check decodepst fee calculation (input values shall only be counted once per UTXO)
        assert_equal(decoded['fee'], created_pst['fee'])
        assert_equal(walletprocesspst_out['complete'], True)
        self.nodes[1].sendrawtransaction(self.nodes[1].finalizepst(walletprocesspst_out['pst'])['hex'])

        self.log.info("Test walletcreatefundedpst fee rate of 10000 sat/vB and 0.1 FRC/kvB produces a total fee at or slightly below -maxtxfee (~0.05290000)")
        res1 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, refheight, {"fee_rate": 10000, "add_inputs": True})
        assert_approx(res1["fee"], 0.055, 0.005)
        res2 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, refheight, {"feeRate": "0.1", "add_inputs": True})
        assert_approx(res2["fee"], 0.055, 0.005)

        self.log.info("Test min fee rate checks with walletcreatefundedpst are bypassed, e.g. a fee_rate under 1 sat/vB is allowed")
        res3 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, refheight, {"fee_rate": "0.999", "add_inputs": True})
        assert_approx(res3["fee"], 0.00000381, 0.0000001)
        res4 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, refheight, {"feeRate": 0.00000999, "add_inputs": True})
        assert_approx(res4["fee"], 0.00000381, 0.0000001)

        self.log.info("Test min fee rate checks with walletcreatefundedpst are bypassed and that funding non-standard 'zero-fee' transactions is valid")
        for param, zero_value in product(["fee_rate", "feeRate"], [0, 0.000, 0.00000000, "0", "0.000", "0.00000000"]):
            assert_equal(0, self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, refheight, {param: zero_value, "add_inputs": True})["fee"])

        self.log.info("Test invalid fee rate settings")
        for param, value in {("fee_rate", 100000), ("feeRate", 1)}:
            assert_raises_rpc_error(-4, "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {param: value, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount out of range",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {param: -1, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount is not a number or string",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {param: {"foo": "bar"}, "add_inputs": True})
            # Test fee rate values that don't pass fixed-point parsing checks.
            for invalid_value in ["", 0.000000001, 1e-09, 1.111111111, 1111111111111111, "31.999999999999999999999"]:
                assert_raises_rpc_error(-3, "Invalid amount",
                    self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {param: invalid_value, "add_inputs": True})
        # Test fee_rate values that cannot be represented in sat/vB.
        for invalid_value in [0.0001, 0.00000001, 0.00099999, 31.99999999, "0.0001", "0.00000001", "0.00099999", "31.99999999"]:
            assert_raises_rpc_error(-3, "Invalid amount",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {"fee_rate": invalid_value, "add_inputs": True})

        self.log.info("- raises RPC error if both feeRate and fee_rate are passed")
        assert_raises_rpc_error(-8, "Cannot specify both fee_rate (sat/vB) and feeRate (FRC/kvB)",
            self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {"fee_rate": 0.1, "feeRate": 0.1, "add_inputs": True})

        self.log.info("- raises RPC error if both feeRate and estimate_mode passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and feeRate",
            self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {"estimate_mode": "economical", "feeRate": 0.1, "add_inputs": True})

        for param in ["feeRate", "fee_rate"]:
            self.log.info("- raises RPC error if both {} and conf_target are passed".format(param))
            assert_raises_rpc_error(-8, "Cannot specify both conf_target and {}. Please provide either a confirmation "
                "target in blocks for automatic fee estimation, or an explicit fee rate.".format(param),
                self.nodes[1].walletcreatefundedpst ,inputs, outputs, 0, refheight, {param: 1, "conf_target": 1, "add_inputs": True})

        self.log.info("- raises RPC error if both fee_rate and estimate_mode are passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and fee_rate",
            self.nodes[1].walletcreatefundedpst ,inputs, outputs, 0, refheight, {"fee_rate": 1, "estimate_mode": "economical", "add_inputs": True})

        self.log.info("- raises RPC error with invalid estimate_mode settings")
        for k, v in {"number": 42, "object": {"foo": "bar"}}.items():
            assert_raises_rpc_error(-3, "Expected type string for estimate_mode, got {}".format(k),
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {"estimate_mode": v, "conf_target": 0.1, "add_inputs": True})
        for mode in ["", "foo", Decimal("3.141592")]:
            assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"',
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {"estimate_mode": mode, "conf_target": 0.1, "add_inputs": True})

        self.log.info("- raises RPC error with invalid conf_target settings")
        for mode in ["unset", "economical", "conservative"]:
            self.log.debug("{}".format(mode))
            for k, v in {"string": "", "object": {"foo": "bar"}}.items():
                assert_raises_rpc_error(-3, "Expected type number for conf_target, got {}".format(k),
                    self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {"estimate_mode": mode, "conf_target": v, "add_inputs": True})
            for n in [-1, 0, 1009]:
                assert_raises_rpc_error(-8, "Invalid conf_target, must be between 1 and 1008",  # max value of 1008 per src/policy/fees.h
                    self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, refheight, {"estimate_mode": mode, "conf_target": n, "add_inputs": True})

        self.log.info("Test walletcreatefundedpst with too-high fee rate produces total fee well above -maxtxfee and raises RPC error")
        # previously this was silently capped at -maxtxfee
        for bool_add, outputs_array in {True: outputs, False: [{self.nodes[1].getnewaddress(): 1}]}.items():
            msg = "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)"
            assert_raises_rpc_error(-4, msg, self.nodes[1].walletcreatefundedpst, inputs, outputs_array, 0, refheight, {"fee_rate": 1000000, "add_inputs": bool_add})
            assert_raises_rpc_error(-4, msg, self.nodes[1].walletcreatefundedpst, inputs, outputs_array, 0, refheight, {"feeRate": 1, "add_inputs": bool_add})

        self.log.info("Test various PST operations")
        # partially sign multisig things with node 1
        pstx = wmulti.walletcreatefundedpst(inputs=[{"txid":txid,"vout":p2wsh_pos},{"txid":txid,"vout":p2sh_pos},{"txid":txid,"vout":p2sh_p2wsh_pos}], outputs={self.nodes[1].getnewaddress():29.99}, lockheight=refheight, options={'changeAddress': self.nodes[1].getrawchangeaddress()})['pst']
        walletprocesspst_out = self.nodes[1].walletprocesspst(pstx)
        pstx = walletprocesspst_out['pst']
        assert_equal(walletprocesspst_out['complete'], False)

        # Unload wmulti, we don't need it anymore
        wmulti.unloadwallet()

        # partially sign with node 2. This should be complete and sendable
        walletprocesspst_out = self.nodes[2].walletprocesspst(pstx)
        assert_equal(walletprocesspst_out['complete'], True)
        self.nodes[2].sendrawtransaction(self.nodes[2].finalizepst(walletprocesspst_out['pst'])['hex'])

        # check that walletprocesspst fails to decode a non-pst
        rawtx = self.nodes[1].createrawtransaction([{"txid":txid,"vout":p2wpkh_pos}], {self.nodes[1].getnewaddress():9.99})
        assert_raises_rpc_error(-22, "TX decode failed", self.nodes[1].walletprocesspst, rawtx)

        # Convert a non-pst to pst and make sure we can decode it
        rawtx = self.nodes[0].createrawtransaction([], {self.nodes[1].getnewaddress():10})
        rawtx = self.nodes[0].fundrawtransaction(rawtx)
        new_pst = self.nodes[0].converttopst(rawtx['hex'])
        self.nodes[0].decodepst(new_pst)

        # Make sure that a non-pst with signatures cannot be converted
        # Error could be either "TX decode failed" (segwit inputs causes parsing to fail) or "Inputs must not have scriptSigs and scriptWitnesses"
        # We must set iswitness=True because the serialized transaction has inputs and is therefore a witness transaction
        signedtx = self.nodes[0].signrawtransactionwithwallet(rawtx['hex'])
        assert_raises_rpc_error(-22, "", self.nodes[0].converttopst, hexstring=signedtx['hex'], iswitness=True)
        assert_raises_rpc_error(-22, "", self.nodes[0].converttopst, hexstring=signedtx['hex'], permitsigdata=False, iswitness=True)
        # Unless we allow it to convert and strip signatures
        self.nodes[0].converttopst(signedtx['hex'], True)

        # Explicitly allow converting non-empty txs
        new_pst = self.nodes[0].converttopst(rawtx['hex'])
        self.nodes[0].decodepst(new_pst)

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
        block_height = self.nodes[0].getblockcount()
        unspent = self.nodes[0].listunspent()[0]
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, unspent["refheight"], {"add_inputs": True}, False)
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NONFINAL)
            assert "bip32_derivs" not in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], block_height+2)

        # Same construction with only locktime set
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height, unspent["refheight"], {"add_inputs": True}, True)
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NONFINAL)
            assert "bip32_derivs" in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], block_height)

        # Same construction without optional arguments
        pstx_info = self.nodes[0].walletcreatefundedpst([], [{self.nodes[2].getnewaddress():unspent["amount"]+1}])
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert tx_in["sequence"] >= MAX_SEQUENCE_NONFINAL
            assert "bip32_derivs" in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], 0)

        # Same construction without optional arguments
        unspent1 = self.nodes[1].listunspent()[0]
        pstx_info = self.nodes[1].walletcreatefundedpst([{"txid":unspent1["txid"], "vout":unspent1["vout"]}], [{self.nodes[2].getnewaddress():unspent1["amount"]+1}], block_height, unspent1["refheight"], {"add_inputs": True})
        decoded_pst = self.nodes[1].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NONFINAL)
            assert "bip32_derivs" in pst_in

        # Make sure change address wallet does not have P2SH innerscript access to results in success
        # when attempting BnB coin selection
        self.nodes[0].walletcreatefundedpst([], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, block_height+2, {"changeAddress":self.nodes[1].getnewaddress()}, False)

        # Make sure the wallet's change type is respected by default
        small_output = {self.nodes[0].getnewaddress():0.1}
        pstx_native = self.nodes[0].walletcreatefundedpst([], [small_output])
        self.assert_change_type(pstx_native, "witness_v0_keyhash")
        pstx_legacy = self.nodes[1].walletcreatefundedpst([], [small_output])
        self.assert_change_type(pstx_legacy, "pubkeyhash")

        # Make sure the change type of the wallet can also be overwritten
        pstx_np2wkh = self.nodes[1].walletcreatefundedpst([], [small_output], 0, 0, {"change_type":"p2sh-segwit"})
        self.assert_change_type(pstx_np2wkh, "scripthash")

        # Make sure the change type cannot be specified if a change address is given
        invalid_options = {"change_type":"legacy","changeAddress":self.nodes[0].getnewaddress()}
        assert_raises_rpc_error(-8, "both change address and address type options", self.nodes[0].walletcreatefundedpst, [], [small_output], 0, 0, invalid_options)

        # Regression test for 14473 (mishandling of already-signed witness transaction):
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], 0, 0, {"add_inputs": True})
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
        wunsafe.walletcreatefundedpst([], [{self.nodes[0].getnewaddress(): 1}], 0, 0, {"include_unsafe": True})

        # BIP 174 Test Vectors

        # Check that unknown values are just passed through
        unknown_pst = "707374ff0100430200000001ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000ffffffff010000000000000000036a01000000000001000000000a0f0102030405060708090f0102030405060708090a0b0c0d0e0f0000"
        unknown_out = self.nodes[0].walletprocesspst(unknown_pst)['pst']
        assert_equal(unknown_pst, unknown_out)

        # Remove the PST json tests, since they are extremely difficult to
        # update to include refheights.
        # FIXME: update the test cases in the data/rpc_pst.json file.
        """
        # Open the data file
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data/rpc_pst.json'), encoding='utf-8') as f:
            d = json.load(f)
            invalids = d['invalid']
            valids = d['valid']
            creators = d['creator']
            signers = d['signer']
            combiners = d['combiner']
            finalizers = d['finalizer']
            extractors = d['extractor']

        # Invalid PSTs
        for invalid in invalids:
            assert_raises_rpc_error(-22, "TX decode failed", self.nodes[0].decodepst, invalid)

        # Valid PSTs
        for valid in valids:
            self.nodes[0].decodepst(valid)

        # Creator Tests
        for creator in creators:
            created_tx = self.nodes[0].createpst(creator['inputs'], creator['outputs'])
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
        """

        # TODO: Re-enable this for segwit v1
        # self.test_utxo_conversion()

        # Test that psts with p2pkh outputs are created properly
        p2pkh = self.nodes[0].getnewaddress(address_type='legacy')
        pst = self.nodes[1].walletcreatefundedpst([], [{p2pkh : 1}], 0, 0, {"includeWatching" : True}, True)
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
        test_pst_input_keys(decoded['inputs'][0], ['witness_utxo'])
        test_pst_input_keys(decoded['inputs'][1], [])
        test_pst_input_keys(decoded['inputs'][2], [])

        # Try again, now while providing descriptors, making P2SH-segwit work, and causing bip32_derivs and redeem_script to be filled in
        descs = [self.nodes[1].getaddressinfo(addr)['desc'] for addr in [addr1,addr2,addr3]]
        updated = self.nodes[1].utxoupdatepst(pst=pst, descriptors=descs)
        decoded = self.nodes[1].decodepst(updated)
        test_pst_input_keys(decoded['inputs'][0], ['witness_utxo', 'bip32_derivs'])
        test_pst_input_keys(decoded['inputs'][1], [])
        test_pst_input_keys(decoded['inputs'][2], ['witness_utxo', 'bip32_derivs', 'redeem_script'])

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
        assert_equal(analyzed['fee'], Decimal('0.001'))
        assert_equal(analyzed['estimated_vsize'], 138)
        assert_equal(analyzed['estimated_feerate'], Decimal('0.00724637'))

        # After signing and finalizing, needs extracting
        signed = self.nodes[1].walletprocesspst(updated)['pst']
        analyzed = self.nodes[0].analyzepst(signed)
        assert analyzed['inputs'][0]['has_utxo'] and analyzed['inputs'][0]['is_final'] and analyzed['next'] == 'extractor'

        self.log.info("PST spending unspendable outputs should have error message and Creator as next")
        analysis = self.nodes[0].analyzepst('707374ff01009e020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f545887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160041d85c2b71d0060b09c9886aeb815e50991dda124d00e1f5050000000016ff3fc000d760fffbffffffffff09c4e8742d1d87008f00000000010000000001012400c2eb0b00000000176a14b7f5faf442e3797da74e87fe7d9d7497e3b2028903010000000107090301071000010000800001012400c2eb0b00000000176a14b7f5faf442e3797da74e87fe7d9d7497e3b20289030100000001070903010710d90c6a4f00000000')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 spends unspendable output')

        self.log.info("PST with invalid values should have error message and Creator as next")
        analysis = self.nodes[0].analyzepst('707374ff0100750200000001f034d01160026eada7ce91a30c07e8130899034be735d47134fded7a0b1b3d010000000000ffffffff0200f902950000000016001428dc34c7c1d172d0209afa1ebe6e2ed526cded72fced029500000000160014f724e2017b88be0d2f8c2d7d100a8106e08624d40000000001000000000101230080816ae3d007001600149503b717f63c7a3ae351c438138022f14c353af601000000000000')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 has invalid value')

        self.log.info("PST with signed, but not finalized, inputs should have Finalizer as next")
        analysis = self.nodes[0].analyzepst('707374ff0100750200000001961ecdcc5d9db5e84029ab0fbf6dfcbcc382d528f4446a8b193cc208a80286eb0000000000fdffffff0250c3000000000000160014cbf531c59bb366cc1c9859cdfc4f431928872d4b2e18f50500000000160014bb07e68d11dfcc17c7e5f6702a2c4cc0999d0e1b68000000010000000001012300e1f50500000000160014e048da1e6d85fdfe2df95bd8b182b15499c2c96d01000000220202fe2e1db550110915ad44e1b41c7a0671a564973254730a398689a01a448d027247304402206c7bcca70c206afb704112f4147829db7fa733d32b77ca59c1b20895dbecc56702200b38182a94c1f819b0b51c387f1e6ca598ee787f2132e2529d7bbfc14ff3d6c70101030401000000220602fe2e1db550110915ad44e1b41c7a0671a564973254730a398689a01a448d0272180f05694354000080010000800000008000000000000000000000220202fe088881e7d3520acb0e2511a231840ad5c85a86001a689a485c4e350b28abfb180f056943540000800100008000000080010000000000000000')
        assert_equal(analysis['next'], 'updater') # <-- should be 'finalizer',
                                                  # but we don't have the
                                                  # privkey necessary to update
                                                  # this test.

        analysis = self.nodes[0].analyzepst('707374ff0100750200000001f034d01160026eada7ce91a30c07e8130899034be735d47134fded7a0b1b3d010000000000ffffffff020080816ae3d0070016001428dc34c7c1d172d0209afa1ebe6e2ed526cded72fced029500000000160014f724e2017b88be0d2f8c2d7d100a8106e08624d400000000010000000001012300f2052a010000001600149503b717f63c7a3ae351c438138022f14c353af601000000000000')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Output amount invalid')

        analysis = self.nodes[0].analyzepst('707374ff01009e02000000023111ba7a42f774121a1d75526572eed38389bcbcfefc4a8cf7b16d7d0ef6215a0300000000ffffffff3111ba7a42f774121a1d75526572eed38389bcbcfefc4a8cf7b16d7d0ef6215a0100000000feffffff02c0b60295000000001600148d24ace36946f7b8ec62c6cbe1d46184ddf5bbcd00f902950000000016001428dc34c7c1d172d0209afa1ebe6e2ed526cded720000000001000000000100a1020000000273331adf6d6d547b8de062962919dcc9c236d9a5f7b9783048550a336a2b8d1b0100000000feffffff73331adf6d6d547b8de062962919dcc9c236d9a5f7b9783048550a336a2b8d1b0000000000feffffff0200f90295000000001976a914fdcd751503c2ece6256aca72c77b9fef49ed57c988ac40cd0295000000001600146659051c8d28e0850f3fb87e019ffce46a4c0bb400000000010000000001012340cd0295000000001600146659051c8d28e0850f3fb87e019ffce46a4c0bb401000000000000')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 specifies invalid prevout')

        assert_raises_rpc_error(-25, 'Inputs missing or spent', self.nodes[0].walletprocesspst, '707374ff01009e02000000023111ba7a42f774121a1d75526572eed38389bcbcfefc4a8cf7b16d7d0ef6215a0300000000ffffffff3111ba7a42f774121a1d75526572eed38389bcbcfefc4a8cf7b16d7d0ef6215a0100000000feffffff02c0b60295000000001600148d24ace36946f7b8ec62c6cbe1d46184ddf5bbcd00f902950000000016001428dc34c7c1d172d0209afa1ebe6e2ed526cded720000000001000000000100a1020000000273331adf6d6d547b8de062962919dcc9c236d9a5f7b9783048550a336a2b8d1b0100000000feffffff73331adf6d6d547b8de062962919dcc9c236d9a5f7b9783048550a336a2b8d1b0000000000feffffff0200f90295000000001976a914fdcd751503c2ece6256aca72c77b9fef49ed57c988ac40cd0295000000001600146659051c8d28e0850f3fb87e019ffce46a4c0bb400000000010000000001012340cd0295000000001600146659051c8d28e0850f3fb87e019ffce46a4c0bb401000000000000')

        self.log.info("Test that we can fund psts with external inputs specified")

        eckey = ECKey()
        eckey.generate()
        privkey = bytes_to_wif(eckey.get_bytes())

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
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.walletcreatefundedpst, [ext_utxo], {self.nodes[0].getnewaddress(): 15})

        # But funding should work when the solving data is provided
        pst = wallet.walletcreatefundedpst([ext_utxo], {self.nodes[0].getnewaddress(): 15}, 0, 0, {"add_inputs": True, "solving_data": {"pubkeys": [addr_info['pubkey']], "scripts": [addr_info["embedded"]["scriptPubKey"], addr_info["embedded"]["embedded"]["scriptPubKey"]]}})
        signed = wallet.walletprocesspst(pst['pst'])
        assert not signed['complete']
        signed = self.nodes[0].walletprocesspst(signed['pst'])
        assert signed['complete']
        self.nodes[0].finalizepst(signed['pst'])

        pst = wallet.walletcreatefundedpst([ext_utxo], {self.nodes[0].getnewaddress(): 15}, 0, 0, {"add_inputs": True, "solving_data":{"descriptors": [desc]}})
        signed = wallet.walletprocesspst(pst['pst'])
        assert not signed['complete']
        signed = self.nodes[0].walletprocesspst(signed['pst'])
        assert signed['complete']
        final = self.nodes[0].finalizepst(signed['pst'], False)

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
            options={"add_inputs": True}
        )
        signed = wallet.walletprocesspst(pst["pst"])
        signed = self.nodes[0].walletprocesspst(signed["pst"])
        final = self.nodes[0].finalizepst(signed["pst"])
        assert self.nodes[0].testmempoolaccept([final["hex"]])[0]["allowed"]
        # Reducing the weight should have a lower fee
        pst2 = wallet.walletcreatefundedpst(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": low_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            options={"add_inputs": True}
        )
        assert_greater_than(pst["fee"], pst2["fee"])
        # Increasing the weight should have a higher fee
        pst2 = wallet.walletcreatefundedpst(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            options={"add_inputs": True}
        )
        assert_greater_than(pst2["fee"], pst["fee"])
        # The provided weight should override the calculated weight when solving data is provided
        pst3 = wallet.walletcreatefundedpst(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            options={'add_inputs': True, "solving_data":{"descriptors": [desc]}}
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
            options={"add_inputs": True}
        )
        assert_equal(pst2["fee"], pst3["fee"])

if __name__ == '__main__':
    PSTTest().main()
