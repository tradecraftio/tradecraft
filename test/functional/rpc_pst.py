#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
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
"""Test the Partially Signed Transaction RPCs.
"""

from decimal import Decimal
from test_framework.messages import MAX_SEQUENCE_NUMBER
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    find_output,
)

import json
import os

# Create one-input, one-output, no-fee transaction:
class PSTTest(FreicoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 3
        self.extra_args = [
            [],
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
        mining_node.generate(nblocks=1)
        self.sync_blocks([mining_node, online_node])

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
        mining_node.generate(1)
        self.sync_blocks([mining_node, online_node])
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

        pstx1 = self.nodes[0].walletcreatefundedpst([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():90}, 0, {"add_inputs": True})['pst']
        assert_equal(len(self.nodes[0].decodepst(pstx1)['tx']['vin']), 2)

        # Inputs argument can be null
        self.nodes[0].walletcreatefundedpst(None, {self.nodes[2].getnewaddress():10})

        # Node 1 should not be able to add anything to it but still return the pstx same as before
        pstx = self.nodes[1].walletprocesspst(pstx1)['pst']
        assert_equal(pstx1, pstx)

        # Sign the transaction and send
        signed_tx = self.nodes[0].walletprocesspst(pstx)['pst']
        final_tx = self.nodes[0].finalizepst(signed_tx)['hex']
        self.nodes[0].sendrawtransaction(final_tx)

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
        self.nodes[0].generate(6)
        self.sync_all()

        # Find the output pos
        p2sh_pos = -1
        p2wsh_pos = -1
        p2wpkh_pos = -1
        p2pkh_pos = -1
        p2sh_p2wsh_pos = -1
        p2sh_p2wpkh_pos = -1
        decoded = self.nodes[0].decoderawtransaction(signed_tx)
        for out in decoded['vout']:
            if out['scriptPubKey']['addresses'][0] == p2sh:
                p2sh_pos = out['n']
            elif out['scriptPubKey']['addresses'][0] == p2wsh:
                p2wsh_pos = out['n']
            elif out['scriptPubKey']['addresses'][0] == p2wpkh:
                p2wpkh_pos = out['n']
            elif out['scriptPubKey']['addresses'][0] == p2sh_p2wsh:
                p2sh_p2wsh_pos = out['n']
            elif out['scriptPubKey']['addresses'][0] == p2sh_p2wpkh:
                p2sh_p2wpkh_pos = out['n']
            elif out['scriptPubKey']['addresses'][0] == p2pkh:
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
        self.nodes[1].sendrawtransaction(self.nodes[1].finalizepst(walletprocesspst_out['pst'])['hex'])

        self.log.info("Test walletcreatefundedpst fee rate of 10000 sat/vB and 0.1 FRC/kvB produces a total fee at or slightly below -maxtxfee (~0.05290000)")
        res1 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {"fee_rate": 10000, "add_inputs": True})
        assert_approx(res1["fee"], 0.055, 0.005)
        res2 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {"feeRate": "0.1", "add_inputs": True})
        assert_approx(res2["fee"], 0.055, 0.005)

        self.log.info("Test min fee rate checks with walletcreatefundedpst are bypassed, e.g. a fee_rate under 1 sat/vB is allowed")
        res3 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {"fee_rate": "0.99999999", "add_inputs": True})
        assert_approx(res3["fee"], 0.00000381, 0.0000001)
        res4 = self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {"feeRate": 0.00000999, "add_inputs": True})
        assert_approx(res4["fee"], 0.00000381, 0.0000001)

        self.log.info("Test min fee rate checks with walletcreatefundedpst are bypassed and that funding non-standard 'zero-fee' transactions is valid")
        for param in ["fee_rate", "feeRate"]:
            assert_equal(self.nodes[1].walletcreatefundedpst(inputs, outputs, 0, {param: 0, "add_inputs": True})["fee"], 0)

        self.log.info("Test invalid fee rate settings")
        for param, value in {("fee_rate", 100000), ("feeRate", 1)}:
            assert_raises_rpc_error(-4, "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {param: value, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount out of range",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {param: -1, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount is not a number or string",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {param: {"foo": "bar"}, "add_inputs": True})
            assert_raises_rpc_error(-3, "Invalid amount",
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {param: "", "add_inputs": True})

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
            assert_raises_rpc_error(-3, "Expected type string for estimate_mode, got {}".format(k),
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"estimate_mode": v, "conf_target": 0.1, "add_inputs": True})
        for mode in ["", "foo", Decimal("3.141592")]:
            assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"',
                self.nodes[1].walletcreatefundedpst, inputs, outputs, 0, {"estimate_mode": mode, "conf_target": 0.1, "add_inputs": True})

        self.log.info("- raises RPC error with invalid conf_target settings")
        for mode in ["unset", "economical", "conservative"]:
            self.log.debug("{}".format(mode))
            for k, v in {"string": "", "object": {"foo": "bar"}}.items():
                assert_raises_rpc_error(-3, "Expected type number for conf_target, got {}".format(k),
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
        pstx = wmulti.walletcreatefundedpst(inputs=[{"txid":txid,"vout":p2wsh_pos},{"txid":txid,"vout":p2sh_pos},{"txid":txid,"vout":p2sh_p2wsh_pos}], outputs={self.nodes[1].getnewaddress():29.99}, options={'changeAddress': self.nodes[1].getrawchangeaddress()})['pst']
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
        blockhash = self.nodes[0].generate(6)[0]
        self.sync_all()
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
        self.nodes[0].generate(6)
        self.sync_all()

        # Test additional args in walletcreatepst
        # Make sure both pre-included and funded inputs
        # have the correct sequence numbers based on
        block_height = self.nodes[0].getblockcount()
        unspent = self.nodes[0].listunspent()[0]
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, {"add_inputs": True}, False)
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NUMBER)
            assert "bip32_derivs" not in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], block_height+2)

        # Same construction with only locktime set
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height, {"add_inputs": True}, True)
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NUMBER)
            assert "bip32_derivs" in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], block_height)

        # Same construction without optional arguments
        pstx_info = self.nodes[0].walletcreatefundedpst([], [{self.nodes[2].getnewaddress():unspent["amount"]+1}])
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert tx_in["sequence"] >= MAX_SEQUENCE_NUMBER
            assert "bip32_derivs" in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], 0)

        # Same construction without optional arguments
        unspent1 = self.nodes[1].listunspent()[0]
        pstx_info = self.nodes[1].walletcreatefundedpst([{"txid":unspent1["txid"], "vout":unspent1["vout"]}], [{self.nodes[2].getnewaddress():unspent1["amount"]+1}], block_height, {"add_inputs": True})
        decoded_pst = self.nodes[1].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NUMBER)
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

        # BIP 174 Test Vectors

        # Check that unknown values are just passed through
        unknown_pst = "cHNidP8BAD8CAAAAAf//////////////////////////////////////////AAAAAAD/////AQAAAAAAAAAAA2oBAAAAAAAACg8BAgMEBQYHCAkPAQIDBAUGBwgJCgsMDQ4PAAA="
        unknown_out = self.nodes[0].walletprocesspst(unknown_pst)['pst']
        assert_equal(unknown_pst, unknown_out)

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
            signed_tx = wrpc.walletprocesspst(signer['pst'])['pst']
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

        # TODO: Re-enable this for segwit v1
        # self.test_utxo_conversion()

        # Test that psts with p2pkh outputs are created properly
        p2pkh = self.nodes[0].getnewaddress(address_type='legacy')
        pst = self.nodes[1].walletcreatefundedpst([], [{p2pkh : 1}], 0, {"includeWatching" : True}, True)
        self.nodes[0].decodepst(pst['pst'])

        # Test decoding error: invalid base64
        assert_raises_rpc_error(-22, "TX decode failed invalid base64", self.nodes[0].decodepst, ";definitely not base64;")

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
        self.nodes[0].generate(6)
        self.sync_all()
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
        blockhash = self.nodes[0].generate(6)[0]
        self.sync_all()
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
        analysis = self.nodes[0].analyzepst('cHNidP8BAJoCAAAAAljoeiG1ba8MI76OcHBFbDNvfLqlyHV5JPVFiHuyq911AAAAAAD/////g40EJ9DsZQpoqka7CwmK6kQiwHGyyng1Kgd5WdB86h0BAAAAAP////8CcKrwCAAAAAAWAEHYXCtx0AYLCcmIauuBXlCZHdoSTQDh9QUAAAAAFv8/wADXYP/7//////8JxOh0LR2HAI8AAAAAAAEBIADC6wsAAAAAF2oUt/X69ELjeX2nTof+fZ10l+OyAokDAQcJAwEHEAABAACAAAEBIADC6wsAAAAAF2oUt/X69ELjeX2nTof+fZ10l+OyAokDAQcJAwEHENkMak8AAAAA')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 spends unspendable output')

        self.log.info("PST with invalid values should have error message and Creator as next")
        analysis = self.nodes[0].analyzepst('cHNidP8BAHECAAAAAfA00BFgAm6tp86RowwH6BMImQNL5zXUcTT97XoLGz0BAAAAAAD/////AgD5ApUAAAAAFgAUKNw0x8HRctAgmvoevm4u1SbN7XL87QKVAAAAABYAFPck4gF7iL4NL4wtfRAKgQbghiTUAAAAAAABAR8AgIFq49AHABYAFJUDtxf2PHo641HEOBOAIvFMNTr2AAAA')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 has invalid value')

        self.log.info("PST with signed, but not finalized, inputs should have Finalizer as next")
        analysis = self.nodes[0].analyzepst('cHNidP8BAHECAAAAAZYezcxdnbXoQCmrD79t/LzDgtUo9ERqixk8wgioAobrAAAAAAD9////AlDDAAAAAAAAFgAUy/UxxZuzZswcmFnN/E9DGSiHLUsuGPUFAAAAABYAFLsH5o0R38wXx+X2cCosTMCZnQ4baAAAAAABAR8A4fUFAAAAABYAFOBI2h5thf3+Lflb2LGCsVSZwsltIgIC/i4dtVARCRWtROG0HHoGcaVklzJUcwo5homgGkSNAnJHMEQCIGx7zKcMIGr7cEES9BR4Kdt/pzPTK3fKWcGyCJXb7MVnAiALOBgqlMH4GbC1HDh/HmylmO54fyEy4lKde7/BT/PWxwEBAwQBAAAAIgYC/i4dtVARCRWtROG0HHoGcaVklzJUcwo5homgGkSNAnIYDwVpQ1QAAIABAACAAAAAgAAAAAAAAAAAAAAiAgL+CIiB59NSCssOJRGiMYQK1chahgAaaJpIXE41Cyir+xgPBWlDVAAAgAEAAIAAAACAAQAAAAAAAAAA')
        assert_equal(analysis['next'], 'finalizer')

        analysis = self.nodes[0].analyzepst('cHNidP8BAHECAAAAAfA00BFgAm6tp86RowwH6BMImQNL5zXUcTT97XoLGz0BAAAAAAD/////AgCAgWrj0AcAFgAUKNw0x8HRctAgmvoevm4u1SbN7XL87QKVAAAAABYAFPck4gF7iL4NL4wtfRAKgQbghiTUAAAAAAABAR8A8gUqAQAAABYAFJUDtxf2PHo641HEOBOAIvFMNTr2AAAA')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Output amount invalid')

        analysis = self.nodes[0].analyzepst('cHNidP8BAJoCAAAAAkvEW8NnDtdNtDpsmze+Ht2LH35IJcKv00jKAlUs21RrAwAAAAD/////S8Rbw2cO1020OmybN74e3Ysffkglwq/TSMoCVSzbVGsBAAAAAP7///8CwLYClQAAAAAWABSNJKzjaUb3uOxixsvh1GGE3fW7zQD5ApUAAAAAFgAUKNw0x8HRctAgmvoevm4u1SbN7XIAAAAAAAEAnQIAAAACczMa321tVHuN4GKWKRncycI22aX3uXgwSFUKM2orjRsBAAAAAP7///9zMxrfbW1Ue43gYpYpGdzJwjbZpfe5eDBIVQozaiuNGwAAAAAA/v///wIA+QKVAAAAABl2qRT9zXUVA8Ls5iVqynLHe5/vSe1XyYisQM0ClQAAAAAWABRmWQUcjSjghQ8/uH4Bn/zkakwLtAAAAAAAAQEfQM0ClQAAAAAWABRmWQUcjSjghQ8/uH4Bn/zkakwLtAAAAA==')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PST is not valid. Input 0 specifies invalid prevout')

        assert_raises_rpc_error(-25, 'Inputs missing or spent', self.nodes[0].walletprocesspst, 'cHNidP8BAJoCAAAAAkvEW8NnDtdNtDpsmze+Ht2LH35IJcKv00jKAlUs21RrAwAAAAD/////S8Rbw2cO1020OmybN74e3Ysffkglwq/TSMoCVSzbVGsBAAAAAP7///8CwLYClQAAAAAWABSNJKzjaUb3uOxixsvh1GGE3fW7zQD5ApUAAAAAFgAUKNw0x8HRctAgmvoevm4u1SbN7XIAAAAAAAEAnQIAAAACczMa321tVHuN4GKWKRncycI22aX3uXgwSFUKM2orjRsBAAAAAP7///9zMxrfbW1Ue43gYpYpGdzJwjbZpfe5eDBIVQozaiuNGwAAAAAA/v///wIA+QKVAAAAABl2qRT9zXUVA8Ls5iVqynLHe5/vSe1XyYisQM0ClQAAAAAWABRmWQUcjSjghQ8/uH4Bn/zkakwLtAAAAAAAAQEfQM0ClQAAAAAWABRmWQUcjSjghQ8/uH4Bn/zkakwLtAAAAA==')

if __name__ == '__main__':
    PSTTest().main()
