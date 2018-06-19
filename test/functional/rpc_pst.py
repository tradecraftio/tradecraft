#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
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
"""Test the Partially Signed Transaction RPCs.
"""

from decimal import Decimal
from test_framework.messages import MAX_SEQUENCE_NUMBER
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal, assert_greater_than, assert_raises_rpc_error, connect_nodes_bi, disconnect_nodes, find_output, sync_blocks

import json
import os

# Create one-input, one-output, no-fee transaction:
class PSTTest(FreicoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 3
        self.extra_args = [
            [],
            [],
            []
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_utxo_conversion(self):
        mining_node = self.nodes[2]
        offline_node = self.nodes[0]
        online_node = self.nodes[1]

        # Disconnect offline node from others
        disconnect_nodes(offline_node, 1)
        disconnect_nodes(online_node, 0)
        disconnect_nodes(offline_node, 2)
        disconnect_nodes(mining_node, 0)

        # Mine a transaction that credits the offline address
        offline_addr = offline_node.getnewaddress(address_type="p2sh-segwit")
        online_addr = online_node.getnewaddress(address_type="p2sh-segwit")
        online_node.importaddress(offline_addr, "", False)
        mining_node.sendtoaddress(address=offline_addr, amount=1.0)
        mining_node.generate(nblocks=1)
        sync_blocks([mining_node, online_node])

        # Construct an unsigned PST on the online node (who doesn't know the output is Segwit, so will include a non-witness UTXO)
        utxos = online_node.listunspent(addresses=[offline_addr])
        raw = online_node.createrawtransaction([{"txid":utxos[0]["txid"], "vout":utxos[0]["vout"]}],[{online_addr:0.9999}])
        pst = online_node.walletprocesspst(online_node.converttopst(raw))["pst"]
        assert("non_witness_utxo" in mining_node.decodepst(pst)["inputs"][0])

        # Have the offline node sign the PST (which will update the UTXO to segwit)
        signed_pst = offline_node.walletprocesspst(pst)["pst"]
        assert("witness_utxo" in mining_node.decodepst(signed_pst)["inputs"][0])

        # Make sure we can mine the resulting transaction
        txid = mining_node.sendrawtransaction(mining_node.finalizepst(signed_pst)["hex"])
        mining_node.generate(1)
        sync_blocks([mining_node, online_node])
        assert_equal(online_node.gettxout(txid,0)["confirmations"], 1)

        # Reconnect
        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)

    def run_test(self):
        # Create and fund a raw tx for sending 10 FRC
        pstx1 = self.nodes[0].walletcreatefundedpst([], {self.nodes[2].getnewaddress():10})['pst']

        # Node 1 should not be able to add anything to it but still return the pstx same as before
        pstx = self.nodes[1].walletprocesspst(pstx1)['pst']
        assert_equal(pstx1, pstx)

        # Sign the transaction and send
        signed_tx = self.nodes[0].walletprocesspst(pstx)['pst']
        final_tx = self.nodes[0].finalizepst(signed_tx)['hex']
        self.nodes[0].sendrawtransaction(final_tx)

        # Create p2sh, p2wpkh, and p2wsh addresses
        pubkey0 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())['pubkey']
        pubkey1 = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())['pubkey']
        pubkey2 = self.nodes[2].getaddressinfo(self.nodes[2].getnewaddress())['pubkey']
        p2sh = self.nodes[1].addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "legacy")['address']
        p2wsh = self.nodes[1].addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "bech32")['address']
        p2sh_p2wsh = self.nodes[1].addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "p2sh-segwit")['address']
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

        # spend single key from node 1
        rawtx = self.nodes[1].walletcreatefundedpst([{"txid":txid,"vout":p2wpkh_pos},{"txid":txid,"vout":p2sh_p2wpkh_pos},{"txid":txid,"vout":p2pkh_pos}], {self.nodes[1].getnewaddress():29.99})['pst']
        walletprocesspst_out = self.nodes[1].walletprocesspst(rawtx)
        assert_equal(walletprocesspst_out['complete'], True)
        self.nodes[1].sendrawtransaction(self.nodes[1].finalizepst(walletprocesspst_out['pst'])['hex'])

        # feeRate of 0.1 FRC / KB produces a total fee slightly below -maxtxfee (~0.05280000):
        res = self.nodes[1].walletcreatefundedpst([{"txid":txid,"vout":p2wpkh_pos},{"txid":txid,"vout":p2sh_p2wpkh_pos},{"txid":txid,"vout":p2pkh_pos}], {self.nodes[1].getnewaddress():29.99}, 0, -1, {"feeRate": 0.095})
        assert_greater_than(res["fee"], 0.05)
        assert_greater_than(0.06, res["fee"])

        # feeRate of 10 FRC / KB produces a total fee well above -maxtxfee
        # previously this was silently capped at -maxtxfee
        assert_raises_rpc_error(-4, "Fee exceeds maximum configured by -maxtxfee", self.nodes[1].walletcreatefundedpst, [{"txid":txid,"vout":p2wpkh_pos},{"txid":txid,"vout":p2sh_p2wpkh_pos},{"txid":txid,"vout":p2pkh_pos}], {self.nodes[1].getnewaddress():29.99}, 0, -1, {"feeRate": 10})

        # partially sign multisig things with node 1
        pstx = self.nodes[1].walletcreatefundedpst([{"txid":txid,"vout":p2wsh_pos},{"txid":txid,"vout":p2sh_pos},{"txid":txid,"vout":p2sh_p2wsh_pos}], {self.nodes[1].getnewaddress():29.99})['pst']
        walletprocesspst_out = self.nodes[1].walletprocesspst(pstx)
        pstx = walletprocesspst_out['pst']
        assert_equal(walletprocesspst_out['complete'], False)

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
        pst1 = self.nodes[1].walletprocesspst(pst_orig)['pst']
        pst1_decoded = self.nodes[0].decodepst(pst1)
        assert pst1_decoded['inputs'][0] and not pst1_decoded['inputs'][1]
        pst2 = self.nodes[2].walletprocesspst(pst_orig)['pst']
        pst2_decoded = self.nodes[0].decodepst(pst2)
        assert not pst2_decoded['inputs'][0] and pst2_decoded['inputs'][1]

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
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, unspent["refheight"], {}, False)
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NUMBER)
            assert "bip32_derivs" not in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], block_height+2)

        # Same construction with only locktime set
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height, unspent["refheight"], {}, True)
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in, pst_in in zip(decoded_pst["tx"]["vin"], decoded_pst["inputs"]):
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NUMBER)
            assert "bip32_derivs" in pst_in
        assert_equal(decoded_pst["tx"]["locktime"], block_height)

        # Same construction without optional arguments
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}])
        decoded_pst = self.nodes[0].decodepst(pstx_info["pst"])
        for tx_in in decoded_pst["tx"]["vin"]:
            assert tx_in["sequence"] >= MAX_SEQUENCE_NUMBER
        assert_equal(decoded_pst["tx"]["locktime"], 0)

        # Same construction without optional arguments
        unspent1 = self.nodes[1].listunspent()[0]
        pstx_info = self.nodes[1].walletcreatefundedpst([{"txid":unspent1["txid"], "vout":unspent1["vout"]}], [{self.nodes[2].getnewaddress():unspent1["amount"]+1}], block_height)
        decoded_pst = self.nodes[1].decodepst(pstx_info["pst"])
        for tx_in in decoded_pst["tx"]["vin"]:
            assert_equal(tx_in["sequence"], MAX_SEQUENCE_NUMBER)

        # Make sure change address wallet does not have P2SH innerscript access to results in success
        # when attempting BnB coin selection
        self.nodes[0].walletcreatefundedpst([], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, block_height+2, {"changeAddress":self.nodes[1].getnewaddress()}, False)

        # Regression test for 14473 (mishandling of already-signed witness transaction):
        pstx_info = self.nodes[0].walletcreatefundedpst([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}])
        complete_pst = self.nodes[0].walletprocesspst(pstx_info["pst"])
        double_processed_pst = self.nodes[0].walletprocesspst(complete_pst["pst"])
        assert_equal(complete_pst, double_processed_pst)
        # We don't care about the decode result, but decoding must succeed.
        self.nodes[0].decodepst(double_processed_pst["pst"])

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
            self.nodes[2].createwallet("wallet{}".format(i))
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
        """

        self.test_utxo_conversion()

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

        # Update a PST with UTXOs from the node
        # Bech32 inputs should be filled with witness UTXO. Other inputs should not be filled because they are non-witness
        pst = self.nodes[1].createpst([{"txid":txid1, "vout":vout1},{"txid":txid2, "vout":vout2},{"txid":txid3, "vout":vout3}], {self.nodes[0].getnewaddress():32.999})
        decoded = self.nodes[1].decodepst(pst)
        assert "witness_utxo" not in decoded['inputs'][0] and "non_witness_utxo" not in decoded['inputs'][0]
        assert "witness_utxo" not in decoded['inputs'][1] and "non_witness_utxo" not in decoded['inputs'][1]
        assert "witness_utxo" not in decoded['inputs'][2] and "non_witness_utxo" not in decoded['inputs'][2]
        updated = self.nodes[1].utxoupdatepst(pst)
        decoded = self.nodes[1].decodepst(updated)
        assert "witness_utxo" in decoded['inputs'][0] and "non_witness_utxo" not in decoded['inputs'][0]
        assert "witness_utxo" not in decoded['inputs'][1] and "non_witness_utxo" not in decoded['inputs'][1]
        assert "witness_utxo" not in decoded['inputs'][2] and "non_witness_utxo" not in decoded['inputs'][2]

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
        for i in range(0, 10):
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
        assert_equal(analyzed['fee'], Decimal('0.001'))
        assert_equal(analyzed['estimated_vsize'], 138)
        assert_equal(analyzed['estimated_feerate'], Decimal('0.00724637'))

        # After signing and finalizing, needs extracting
        signed = self.nodes[1].walletprocesspst(updated)['pst']
        analyzed = self.nodes[0].analyzepst(signed)
        assert analyzed['inputs'][0]['has_utxo'] and analyzed['inputs'][0]['is_final'] and analyzed['next'] == 'extractor'

if __name__ == '__main__':
    PSTTest().main()
