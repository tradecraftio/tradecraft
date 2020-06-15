#!/usr/bin/env python3
# Copyright (c) 2016 The Freicoin developers
# Copyright (c) 2010-2021 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>

#
# Test the SegWit changeover logic
#

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import *
from test_framework.mininode import sha256, ripemd160, fastHash256, CTransaction, CTxIn, COutPoint, CTxOut
from test_framework.address import script_to_p2sh, key_to_p2pkh
from test_framework.script import CScript, OP_HASH160, OP_CHECKSIG, OP_0, hash160, hash256, OP_TRUE, OP_EQUAL, OP_DUP, OP_EQUALVERIFY, OP_1, OP_2, OP_CHECKMULTISIG
from io import BytesIO
from test_framework.mininode import FromHex

NODE_0 = 0
NODE_1 = 1
NODE_2 = 2
WIT_V0 = 0
WIT_V1 = 1

def witness_script(version, pubkey):
    if (version == 0):
        scripthash = bytes_to_hex_str(ripemd160(hash256(hex_str_to_bytes('0021' + pubkey + 'ac'))))
        pkscript = "0014" + scripthash
    elif (version == 1):
        # 1-of-1 multisig
        scripthash = bytes_to_hex_str(hash256(hex_str_to_bytes("005121" + pubkey + "51ae")))
        pkscript = "0020" + scripthash
    else:
        assert("Wrong version" == "0 or 1")
    return pkscript

def addlength(script):
    scriptlen = format(len(script)//2, 'x')
    assert(len(scriptlen) == 2)
    return scriptlen + script

def create_witnessprogram(version, node, utxo, pubkey, amount):
    pkscript = witness_script(version, pubkey);
    inputs = []
    outputs = {}
    inputs.append({ "txid" : utxo["txid"], "vout" : utxo["vout"]} )
    DUMMY_P2SH = "2MySexEGVzZpRgNQ1JdjdP5bRETznm3roQ2" # P2SH of "OP_1 OP_DROP"
    outputs[DUMMY_P2SH] = amount
    tx_to_witness = node.createrawtransaction(inputs,outputs)
    #replace dummy output with our own
    tx_to_witness = tx_to_witness[0:110] + addlength(pkscript) + tx_to_witness[-16:]
    return tx_to_witness

def send_to_witness(version, node, utxo, pubkey, amount, sign=True, insert_redeem_script=""):
    tx_to_witness = create_witnessprogram(version, node, utxo, pubkey, amount)
    if (sign):
        signed = node.signrawtransaction(tx_to_witness)
        assert("errors" not in signed or len(["errors"]) == 0)
        return node.sendrawtransaction(signed["hex"])
    else:
        if (insert_redeem_script):
            tx_to_witness = tx_to_witness[0:82] + addlength(insert_redeem_script) + tx_to_witness[84:]

    return node.sendrawtransaction(tx_to_witness)

def getutxo(txid):
    utxo = {}
    utxo["vout"] = 0
    utxo["txid"] = txid
    return utxo

def find_unspent(node, min_value):
    for utxo in node.listunspent():
        if utxo['amount'] >= min_value:
            return utxo

class SegWitTest(FreicoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3, bitcoinmode=self.options.bitcoinmode)

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-logtimemicros", "-debug", "-walletprematurewitness", "-rpcserialversion=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-logtimemicros", "-debug", "-blockversion=4", "-promiscuousmempoolflags=517", "-prematurewitness", "-walletprematurewitness", "-rpcserialversion=1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-logtimemicros", "-debug", "-blockversion=536870915", "-promiscuousmempoolflags=517", "-prematurewitness", "-walletprematurewitness"]))
        connect_nodes(self.nodes[1], 0)
        connect_nodes(self.nodes[2], 1)
        connect_nodes(self.nodes[0], 2)
        self.is_network_split = False
        self.sync_all()

    def success_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], Decimal("49.998"), sign, redeem_script)
        has_block_final = "finaltx" in node.getblocktemplate({'rules':['segwit','auxpow']})
        block = node.generate(1)
        assert_equal(len(node.getblock(block[0])["tx"]), 2 + has_block_final)
        sync_blocks(self.nodes)

    def skip_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], Decimal("49.998"), sign, redeem_script)
        has_block_final = "finaltx" in node.getblocktemplate({'rules':['segwit','auxpow']})
        block = node.generate(1)
        assert_equal(len(node.getblock(block[0])["tx"]), 1 + has_block_final)
        sync_blocks(self.nodes)

    def fail_accept(self, node, txid, sign, redeem_script=""):
        try:
            send_to_witness(1, node, getutxo(txid), self.pubkey[0], Decimal("49.998"), sign, redeem_script)
        except JSONRPCException as exp:
            assert(exp.error["code"] == -26)
        else:
            raise AssertionError("Tx should not have been accepted")

    def fail_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], Decimal("49.998"), sign, redeem_script)
        try:
            node.generate(1)
        except JSONRPCException as exp:
            assert(exp.error["code"] == -1)
        else:
            raise AssertionError("Created valid block when TestBlockValidity should have failed")
        sync_blocks(self.nodes)

    def run_test(self):
        self.nodes[0].generate(161) #block 161

        print("Verify sigops are counted in GBT with pre-BIP141 rules before the fork")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tmpl = self.nodes[0].getblocktemplate({})
        assert(tmpl['sigoplimit'] == 20000)
        assert(tmpl['transactions'][0]['hash'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 2)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit']})
        assert(tmpl['sigoplimit'] == 20000)
        assert(tmpl['transactions'][0]['hash'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 2)
        self.nodes[0].generate(1) #block 162

        balance_presetup = self.nodes[0].getbalance()
        self.pubkey = []
        wit_ids = [] # wit_ids[NODE][VER] is an array of txids that spend to a witness version VER pkscript to an address for NODE
        for i in range(3):
            newaddress = self.nodes[i].getnewaddress()
            self.pubkey.append(self.nodes[i].validateaddress(newaddress)["pubkey"])
            multiaddress = self.nodes[i].addmultisigaddress(1, [self.pubkey[-1]])
            self.nodes[i].addwitnessaddress(newaddress)
            self.nodes[i].addwitnessaddress(multiaddress)
            wit_ids.append([])
            for v in range(2):
                wit_ids[i].append([])

        for i in range(10):
            for n in range(3):
                for v in range(2):
                    wit_ids[n][v].append(send_to_witness(v, self.nodes[0], find_unspent(self.nodes[0], 50), self.pubkey[n], Decimal("49.999")))

        self.nodes[0].generate(1) #block 163
        sync_blocks(self.nodes)

        # Make sure all nodes recognize the transactions as theirs
        assert_equal(self.nodes[0].getbalance(), balance_presetup - 60*50 + 20*Decimal("49.999") + 50)
        assert_equal(self.nodes[1].getbalance(), 20*Decimal("49.999"))
        assert_equal(self.nodes[2].getbalance(), 20*Decimal("49.999"))

        self.nodes[0].generate(404) #block 423 (+ 144)
        sync_blocks(self.nodes)

        print("Verify default node can't accept any witness format txs before fork")
        # unsigned, no scriptsig
        self.fail_accept(self.nodes[0], wit_ids[NODE_0][WIT_V0][0], False)
        self.fail_accept(self.nodes[0], wit_ids[NODE_0][WIT_V1][0], False)
        # signed
        self.fail_accept(self.nodes[0], wit_ids[NODE_0][WIT_V0][0], True)
        self.fail_accept(self.nodes[0], wit_ids[NODE_0][WIT_V1][0], True)

        print("Verify witness txs are skipped for mining before the fork")
        self.skip_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][0], True) #block 424
        self.skip_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][0], True) #block 425
        self.nodes[2].generate(1) #block 426
        self.nodes[2].generate(1) #block 427

        # TODO: An old node would see these txs without witnesses and be able to mine them

        print("Verify unsigned bare witness txs in versionbits-setting blocks are valid before the fork")
        self.success_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][1], False) #block 428
        self.success_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][1], False) #block 429

        print("Verify unsigned p2sh witness txs with a redeem script in versionbits-settings blocks are valid before the fork")
        self.nodes[2].generate(1) #block 430
        self.nodes[2].generate(1) #block 431

        print("Verify previous witness txs skipped for mining can now be mined")
        assert_equal(len(self.nodes[2].getrawmempool()), 2)
        block = self.nodes[2].generate(1) #block 432 (first block with new rules; 432 = 144 * 3)
        sync_blocks(self.nodes)
        assert_equal(len(self.nodes[2].getrawmempool()), 0)
        segwit_tx_list = self.nodes[2].getblock(block[0])["tx"][:-1]
        assert_equal(len(segwit_tx_list), 3)

        print("Verify block and transaction serialization rpcs return differing serializations depending on rpc serialization flag")
        assert(self.nodes[2].getblock(block[0], False) !=  self.nodes[0].getblock(block[0], False))
        assert(self.nodes[1].getblock(block[0], False) ==  self.nodes[2].getblock(block[0], False))
        for i in range(len(segwit_tx_list)):
            tx = FromHex(CTransaction(), self.nodes[2].gettransaction(segwit_tx_list[i])["hex"])
            assert(self.nodes[2].getrawtransaction(segwit_tx_list[i]) != self.nodes[0].getrawtransaction(segwit_tx_list[i]))
            assert(self.nodes[1].getrawtransaction(segwit_tx_list[i], 0) == self.nodes[2].getrawtransaction(segwit_tx_list[i]))
            assert(self.nodes[0].getrawtransaction(segwit_tx_list[i]) != self.nodes[2].gettransaction(segwit_tx_list[i])["hex"])
            assert(self.nodes[1].getrawtransaction(segwit_tx_list[i]) == self.nodes[2].gettransaction(segwit_tx_list[i])["hex"])
            assert(self.nodes[0].getrawtransaction(segwit_tx_list[i]) == bytes_to_hex_str(tx.serialize_without_witness()))

        print("Verify witness txs without witness data are invalid after the fork")
        self.fail_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][2], False)
        self.fail_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][2], False)

        print("Verify default node can now use witness txs")
        self.success_mine(self.nodes[0], wit_ids[NODE_0][WIT_V0][0], True) #block 432
        self.success_mine(self.nodes[0], wit_ids[NODE_0][WIT_V1][0], True) #block 433
        self.nodes[0].generate(1) #block 434
        self.nodes[0].generate(1) #block 435

        print("Verify sigops are counted in GBT with BIP141 rules after the fork")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit','auxpow']})
        assert(tmpl['sigoplimit'] == 80000)
        assert(tmpl['transactions'][0]['txid'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 8)

        print("Verify non-segwit miners get a valid GBT response after the fork")
        send_to_witness(1, self.nodes[0], find_unspent(self.nodes[0], 50), self.pubkey[0], Decimal("49.998"))
        try:
            tmpl = self.nodes[0].getblocktemplate({})
            assert(len(tmpl['transactions']) == 1)  # Doesn't include witness tx
            assert(tmpl['sigoplimit'] == 20000)
            assert(tmpl['transactions'][0]['hash'] == txid)
            assert(tmpl['transactions'][0]['sigops'] == 2)
            assert(('!segwit' in tmpl['rules']) or ('segwit' not in tmpl['rules']))
        except JSONRPCException:
            # This is an acceptable outcome
            pass

        print("Verify behaviour of importaddress, addwitnessaddress and listunspent")

        # Some public keys to be used later
        pubkeys = [
            "0363D44AABD0F1699138239DF2F042C3282C0671CC7A76826A55C8203D90E39242", # cPiM8Ub4heR9NBYmgVzJQiUH1if44GSBGiqaeJySuL2BKxubvgwb
            "02D3E626B3E616FC8662B489C123349FECBFC611E778E5BE739B257EAE4721E5BF", # cPpAdHaD6VoYbW78kveN2bsvb45Q7G5PhaPApVUGwvF8VQ9brD97
            "04A47F2CBCEFFA7B9BCDA184E7D5668D3DA6F9079AD41E422FA5FD7B2D458F2538A62F5BD8EC85C2477F39650BD391EA6250207065B2A81DA8B009FC891E898F0E", # 91zqCU5B9sdWxzMt1ca3VzbtVm2YM6Hi5Rxn4UDtxEaN9C9nzXV
            "02A47F2CBCEFFA7B9BCDA184E7D5668D3DA6F9079AD41E422FA5FD7B2D458F2538", # cPQFjcVRpAUBG8BA9hzr2yEzHwKoMgLkJZBBtK9vJnvGJgMjzTbd
            "036722F784214129FEB9E8129D626324F3F6716555B603FFE8300BBCB882151228", # cQGtcm34xiLjB1v7bkRa4V3aAc9tS2UTuBZ1UnZGeSeNy627fN66
            "0266A8396EE936BF6D99D17920DB21C6C7B1AB14C639D5CD72B300297E416FD2EC", # cTW5mR5M45vHxXkeChZdtSPozrFwFgmEvTNnanCW6wrqwaCZ1X7K
            "0450A38BD7F0AC212FEBA77354A9B036A32E0F7C81FC4E0C5ADCA7C549C4505D2522458C2D9AE3CEFD684E039194B72C8A10F9CB9D4764AB26FCC2718D421D3B84", # 92h2XPssjBpsJN5CqSP7v9a7cf2kgDunBC6PDFwJHMACM1rrVBJ
        ]

        # Import a compressed key and an uncompressed key, generate some multisig addresses
        self.nodes[0].importprivkey("92e6XLo5jVAVwrQKPNTs93oQco8f8sDNBcpv73Dsrs397fQtFQn")
        uncompressed_spendable_address = ["mvozP4UwyGD2mGZU4D2eMvMLPB9WkMmMQu"]
        self.nodes[0].importprivkey("cNC8eQ5dg3mFAVePDX4ddmPYpPbw41r9bm2jd1nLJT77e6RrzTRR")
        compressed_spendable_address = ["mmWQubrDomqpgSYekvsU7HWEVjLFHAakLe"]
        assert ((self.nodes[0].validateaddress(uncompressed_spendable_address[0])['iscompressed'] == False))
        assert ((self.nodes[0].validateaddress(compressed_spendable_address[0])['iscompressed'] == True))

        self.nodes[0].importpubkey(pubkeys[0])
        compressed_solvable_address = [key_to_p2pkh(pubkeys[0])]
        self.nodes[0].importpubkey(pubkeys[1])
        compressed_solvable_address.append(key_to_p2pkh(pubkeys[1]))
        self.nodes[0].importpubkey(pubkeys[2])
        uncompressed_solvable_address = [key_to_p2pkh(pubkeys[2])]

        spendable_anytime = []                      # These outputs should be seen anytime after importprivkey and addmultisigaddress
        spendable_after_importaddress = []          # These outputs should be seen after importaddress
        solvable_after_importaddress = []           # These outputs should be seen after importaddress but not spendable
        unsolvable_after_importaddress = []         # These outputs should be unsolvable after importaddress
        solvable_anytime = []                       # These outputs should be solvable after importpubkey
        unseen_anytime = []                         # These outputs should never be seen

        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], uncompressed_spendable_address[0]]))
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], uncompressed_solvable_address[0]]))
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_solvable_address[0]]))
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_solvable_address[0], compressed_solvable_address[1]]))
        unknown_address = ["mtKKyoHabkk6e4ppT7NaM7THqPUt7AzPrT", "2NDP3jLWAFT8NDAiUa9qiE6oBt2awmMq7Dx"]

        # Test multisig_without_privkey
        # We have 2 public keys without private keys, use addmultisigaddress to add to wallet.
        # Money sent to P2SH of multisig of this should only be seen after importaddress with the BASE58 P2SH address.

        multisig_without_privkey_address = self.nodes[0].addmultisigaddress(2, [pubkeys[3], pubkeys[4]])
        script = CScript([OP_2, hex_str_to_bytes(pubkeys[3]), hex_str_to_bytes(pubkeys[4]), OP_2, OP_CHECKMULTISIG])
        solvable_after_importaddress.append(CScript([OP_HASH160, hash160(script), OP_EQUAL]))

        for i in compressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                # bare and p2sh multisig with compressed keys should always be spendable
                spendable_anytime.extend([bare, p2sh])
                # P2WSH multisig with compressed keys are spendable after direct importaddress
                spendable_after_importaddress.extend([p2wsh_long, p2wsh_short, p2mast_long, p2mast_short])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with compressed keys should always be spendable
                spendable_anytime.extend([p2pk, p2pkh])
                # P2SH_P2PK, P2SH_P2PKH, and witness with compressed keys are spendable after direct importaddress
                spendable_after_importaddress.extend([p2sh_p2pk, p2wpk_long, p2wpk_short, p2sh_p2pkh, p2wpk_mast_long, p2wpk_mast_short])
                # Non-standard scripts won't be recognized
                unseen_anytime.extend([p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short])

        for i in uncompressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                # bare and p2sh multisig with uncompressed keys should always be spendable
                spendable_anytime.extend([bare, p2sh])
                # P2WSH multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh_long, p2wsh_short, p2mast_long, p2mast_short])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                # P2PK and P2PKH with uncompressed keys should always be spendable
                spendable_anytime.extend([p2pk, p2pkh])
                # P2SH_P2PK and P2SH_P2PKH are spendable after direct importaddress
                spendable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short])

        for i in compressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                # Multisig without private is not seen after addmultisigaddress, but seen after importaddress
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                solvable_after_importaddress.extend([bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                # P2PK and P2PKH with compressed keys should always be seen
                solvable_anytime.extend([p2pk, p2pkh])
                # P2SH_P2PK, P2SH_P2PKH, and witness with compressed keys are seen after direct importaddress
                solvable_after_importaddress.extend([p2sh_p2pk, p2wpk_long, p2wpk_short, p2sh_p2pkh, p2wpk_mast_long, p2wpk_mast_short])
                # Non-standard scripts won't be recognized
                unseen_anytime.extend([p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short])

        for i in uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                # Base uncompressed multisig without private is not seen after addmultisigaddress, but seen after importaddress
                solvable_after_importaddress.extend([bare, p2sh])
                # P2WSH multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh_long, p2wsh_short, p2mast_long, p2mast_short])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                # P2PK and P2PKH with uncompressed keys should always be seen
                solvable_anytime.extend([p2pk, p2pkh])
                # P2SH_P2PK, P2SH_P2PKH with uncompressed keys are seen after direct importaddress
                solvable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short])

        op1 = CScript([OP_1])
        op0 = CScript([OP_0])
        # 2N7MGY19ti4KDMSzRfPAssP6Pxyuxoi6jLe is the P2SH(P2PKH) version of mjoE3sSrb8ByYEvgnC3Aox86u1CHnfJA4V
        unsolvable_address = ["mjoE3sSrb8ByYEvgnC3Aox86u1CHnfJA4V", "2N7MGY19ti4KDMSzRfPAssP6Pxyuxoi6jLe", script_to_p2sh(op1), script_to_p2sh(op0)]
        unsolvable_address_key = hex_str_to_bytes("02341AEC7587A51CDE5279E0630A531AEA2615A9F80B17E8D9376327BAEAA59E3D")
        unsolvablep2pkh = CScript([OP_DUP, OP_HASH160, hash160(unsolvable_address_key), OP_EQUALVERIFY, OP_CHECKSIG])
        unsolvablep2wshp2pkh = CScript([OP_0, hash256(bytes([0]) + unsolvablep2pkh)])
        p2shop0 = CScript([OP_HASH160, hash160(op0), OP_EQUAL])
        p2wshop1 = CScript([OP_0, hash256(bytes([0]) + op1)])
        unsolvable_after_importaddress.append(unsolvablep2pkh)
        unsolvable_after_importaddress.append(unsolvablep2wshp2pkh)
        unsolvable_after_importaddress.append(op1) # OP_1 will be imported as script
        unsolvable_after_importaddress.append(p2wshop1)
        unseen_anytime.append(op0) # OP_0 will be imported as P2SH address with no script provided
        unsolvable_after_importaddress.append(p2shop0)

        spendable_txid = []
        solvable_txid = []
        spendable_txid.append(self.mine_and_test_listunspent(spendable_anytime, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_anytime, 1))
        self.mine_and_test_listunspent(spendable_after_importaddress + solvable_after_importaddress + unseen_anytime + unsolvable_after_importaddress, 0)

        importlist = []
        for i in compressed_spendable_address + uncompressed_spendable_address + compressed_solvable_address + uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                bare = hex_str_to_bytes(v['hex'])
                importlist.append(bytes_to_hex_str(bare))
            else:
                pubkey = hex_str_to_bytes(v['pubkey'])
                p2pk = CScript([pubkey, OP_CHECKSIG])
                p2pkh = CScript([OP_DUP, OP_HASH160, hash160(pubkey), OP_EQUALVERIFY, OP_CHECKSIG])
                importlist.append(bytes_to_hex_str(p2pk))
                importlist.append(bytes_to_hex_str(p2pkh))

        importlist.append(bytes_to_hex_str(unsolvablep2pkh))
        importlist.append(bytes_to_hex_str(unsolvablep2wshp2pkh))
        importlist.append(bytes_to_hex_str(op1))
        importlist.append(bytes_to_hex_str(p2wshop1))

        for i in importlist:
            try:
                self.nodes[0].importaddress(i,"",False,True)
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "The wallet already contains the private key for this address or script")
        for i in compressed_spendable_address + compressed_solvable_address:
            self.nodes[0].addwitnessaddress(i)
            v = self.nodes[0].validateaddress(i)
            if v['isscript']:
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                mastscript = self.nodes[0].addwitnessaddress(i, bytes_to_hex_str(proof))
                assert_equal(bytes_to_hex_str(p2mast_long), mastscript['longhash'])
                assert_equal(bytes_to_hex_str(p2mast_short), mastscript['shorthash'])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                mastscript = self.nodes[0].addwitnessaddress(i, bytes_to_hex_str(proof))
                assert_equal(bytes_to_hex_str(p2wpk_mast_long), mastscript['longhash'])
                assert_equal(bytes_to_hex_str(p2wpk_mast_short), mastscript['shorthash'])
        for i in uncompressed_spendable_address + uncompressed_solvable_address:
            try:
                self.nodes[0].addwitnessaddress(i)
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "Public key or redeemscript not known to wallet, or the key is uncompressed")

        self.nodes[0].importaddress(script_to_p2sh(op0)) # import OP_0 as address only
        self.nodes[0].importaddress(multisig_without_privkey_address) # Test multisig_without_privkey

        spendable_txid.append(self.mine_and_test_listunspent(spendable_anytime + spendable_after_importaddress, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_anytime + solvable_after_importaddress, 1))
        self.mine_and_test_listunspent(unsolvable_after_importaddress, 1)
        self.mine_and_test_listunspent(unseen_anytime, 0)

        # addwitnessaddress should refuse to return a witness address if an uncompressed key is used or the address is
        # not in the wallet
        # note that no witness address should be returned by unsolvable addresses
        # the multisig_without_privkey_address will fail because its keys were not added with importpubkey
        for i in uncompressed_spendable_address + uncompressed_solvable_address + unknown_address + unsolvable_address + [multisig_without_privkey_address]:
            try:
                self.nodes[0].addwitnessaddress(i)
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "Public key or redeemscript not known to wallet, or the key is uncompressed")
            else:
                assert(False)

        for i in compressed_spendable_address + compressed_solvable_address:
            witscript = self.nodes[0].addwitnessaddress(i)
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                assert_equal(bytes_to_hex_str(p2wsh_long), witscript['longhash'])
                assert_equal(bytes_to_hex_str(p2wsh_short), witscript['shorthash'])
                mastscript = self.nodes[0].addwitnessaddress(i, bytes_to_hex_str(proof))
                assert_equal(bytes_to_hex_str(p2mast_long), mastscript['longhash'])
                assert_equal(bytes_to_hex_str(p2mast_short), mastscript['shorthash'])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                assert_equal(bytes_to_hex_str(p2wpk_long), witscript['longhash'])
                assert_equal(bytes_to_hex_str(p2wpk_short), witscript['shorthash'])
                mastscript = self.nodes[0].addwitnessaddress(i, bytes_to_hex_str(proof))
                assert_equal(bytes_to_hex_str(p2wpk_mast_long), mastscript['longhash'])
                assert_equal(bytes_to_hex_str(p2wpk_mast_short), mastscript['shorthash'])

        spendable_txid.append(self.mine_and_test_listunspent(spendable_anytime + spendable_after_importaddress, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_anytime + solvable_after_importaddress, 1))
        self.mine_and_test_listunspent(unsolvable_after_importaddress, 1)
        self.mine_and_test_listunspent(unseen_anytime, 0)

        # Repeat some tests. This time we don't add witness scripts with importaddress
        # Import a compressed key and an uncompressed key, generate some multisig addresses
        self.nodes[0].importprivkey("927pw6RW8ZekycnXqBQ2JS5nPyo1yRfGNN8oq74HeddWSpafDJH")
        uncompressed_spendable_address = ["mguN2vNSCEUh6rJaXoAVwY3YZwZvEmf5xi"]
        self.nodes[0].importprivkey("cMcrXaaUC48ZKpcyydfFo8PxHAjpsYLhdsp6nmtB3E2ER9UUHWnw")
        compressed_spendable_address = ["n1UNmpmbVUJ9ytXYXiurmGPQ3TRrXqPWKL"]

        self.nodes[0].importpubkey(pubkeys[5])
        compressed_solvable_address = [key_to_p2pkh(pubkeys[5])]
        self.nodes[0].importpubkey(pubkeys[6])
        uncompressed_solvable_address = [key_to_p2pkh(pubkeys[6])]

        spendable_addr = "n1UNmpmbVUJ9ytXYXiurmGPQ3TRrXqPWKL"
        spendable_p2pk = CScript([hex_str_to_bytes('03969aec6b6d14f5a9df0d5798c7f48313b25f9bcbcf0412d78aa4630ba3431322'), OP_CHECKSIG])

        solvable_addr = "mhc6QdkYLpHs6xtDFcT9EdZyTLfYzjAu5d"
        solvable_p2pk = CScript([hex_str_to_bytes('0266a8396ee936bf6d99d17920db21c6c7b1ab14c639d5cd72b300297e416fd2ec'), OP_CHECKSIG])

        # cNWMP98SkGEATuc7cgEkNc53kcj8Qh49uabwoLEQqH75f8KctGz4
        unknown_addr = "mgKXrVrcemF35xmr4VUVT2JaKJYqbQGUH8"
        unknown_p2pk = CScript([hex_str_to_bytes('022d3a864020ef2d34c7488ad22b5f30677fbb01c0de4cf663eca7730f565d2e79'), OP_CHECKSIG])

        spendable_after_addwitnessaddress = []      # These outputs should be seen after importaddress
        solvable_after_addwitnessaddress=[]         # These outputs should be seen after importaddress but not spendable
        unseen_anytime = []                         # These outputs should never be seen

        # MAST[spendable_p2pk, solvable_p2pk]
        mast = fastHash256(hash256(bytes([0]) + spendable_p2pk),
                           hash256(bytes([0]) + solvable_p2pk))
        p2mast_long = CScript([OP_0, mast])
        p2mast_short = CScript([OP_0, ripemd160(mast)])
        # FIXME: This combination should be spendable, but what
        #        actually happens is that the second script shadows
        #        the first.  Once this is fixed, change this to
        #        `spendable_after_addwitnessaddress`:
        solvable_after_addwitnessaddress.extend([p2mast_long, p2mast_short])
        # MAST[solvable_p2pk, spendable_p2pk]
        mast = fastHash256(hash256(bytes([0]) + solvable_p2pk),
                           hash256(bytes([0]) + spendable_p2pk))
        p2mast_long = CScript([OP_0, mast])
        p2mast_short = CScript([OP_0, ripemd160(mast)])
        spendable_after_addwitnessaddress.extend([p2mast_long, p2mast_short])
        # MAST[spendable_p2pk, unknown_p2pk]
        mast = fastHash256(hash256(bytes([0]) + spendable_p2pk),
                           hash256(bytes([0]) + unknown_p2pk))
        p2mast_long = CScript([OP_0, mast])
        p2mast_short = CScript([OP_0, ripemd160(mast)])
        spendable_after_addwitnessaddress.extend([p2mast_long, p2mast_short])
        # MAST[unknown_p2pk, spendable_p2pk]
        mast = fastHash256(hash256(bytes([0]) + unknown_p2pk),
                           hash256(bytes([0]) + spendable_p2pk))
        p2mast_long = CScript([OP_0, mast])
        p2mast_short = CScript([OP_0, ripemd160(mast)])
        spendable_after_addwitnessaddress.extend([p2mast_long, p2mast_short])
        # MAST[solvable_p2pk, unknown_p2pk]
        mast = fastHash256(hash256(bytes([0]) + solvable_p2pk),
                           hash256(bytes([0]) + unknown_p2pk))
        p2mast_long = CScript([OP_0, mast])
        p2mast_short = CScript([OP_0, ripemd160(mast)])
        solvable_after_addwitnessaddress.extend([p2mast_long, p2mast_short])
        # MAST[unknown_p2pk, solvable_p2pk]
        mast = fastHash256(hash256(bytes([0]) + unknown_p2pk),
                           hash256(bytes([0]) + solvable_p2pk))
        p2mast_long = CScript([OP_0, mast])
        p2mast_short = CScript([OP_0, ripemd160(mast)])
        solvable_after_addwitnessaddress.extend([p2mast_long, p2mast_short])

        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], uncompressed_spendable_address[0]]))
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_solvable_address[0], uncompressed_solvable_address[0]]))
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_solvable_address[0]]))

        premature_witaddress = []

        for i in compressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                # P2WSH multisig with compressed keys are spendable after addwitnessaddress
                spendable_after_addwitnessaddress.extend([p2wsh_long, p2wsh_short, p2mast_long, p2mast_short])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                # P2WPK are spendable after addwitnessaddress
                spendable_after_addwitnessaddress.extend([p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short])
                # Witness with non-standard P2WPKH are never seen
                unseen_anytime.extend([p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short])

        for i in uncompressed_spendable_address + uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh_long, p2wsh_short, p2mast_long, p2mast_short])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                # Witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short])

        for i in compressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                # P2WSH multisig without private key are seen after addwitnessaddress
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                solvable_after_addwitnessaddress.extend([p2wsh_long, p2wsh_short, p2mast_long, p2mast_short])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                # P2WPK with compressed keys are seen after addwitnessaddress
                solvable_after_addwitnessaddress.extend([p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short])
                # Witness with non-standard P2WPKH are never seen
                unseen_anytime.extend([p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short])

        self.mine_and_test_listunspent(spendable_after_addwitnessaddress + solvable_after_addwitnessaddress + unseen_anytime, 0)

        # addwitnessaddress should refuse to return a witness address if an uncompressed key is used
        # note that a multisig address returned by addmultisigaddress is not solvable until it is added with importaddress
        # premature_witaddress are not accepted until the script is added with addwitnessaddress first
        for i in uncompressed_spendable_address + uncompressed_solvable_address + premature_witaddress + [compressed_solvable_address[1]]:
            try:
                self.nodes[0].addwitnessaddress(i)
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "Public key or redeemscript not known to wallet, or the key is uncompressed")
            else:
                assert(False)

        # after importaddress it should pass addwitnessaddress
        v = self.nodes[0].validateaddress(compressed_solvable_address[1])
        self.nodes[0].importaddress(v['hex'],"",False,True)
        for i in compressed_spendable_address + compressed_solvable_address + premature_witaddress:
            witscript = self.nodes[0].addwitnessaddress(i)
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof] = self.p2sh_address_to_script(v)
                assert_equal(bytes_to_hex_str(p2wsh_long), witscript['longhash'])
                assert_equal(bytes_to_hex_str(p2wsh_short), witscript['shorthash'])
                mastscript = self.nodes[0].addwitnessaddress(i, bytes_to_hex_str(proof))
                assert_equal(bytes_to_hex_str(p2mast_long), mastscript['longhash'])
                assert_equal(bytes_to_hex_str(p2mast_short), mastscript['shorthash'])
            else:
                [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof] = self.p2pkh_address_to_script(v)
                assert_equal(bytes_to_hex_str(p2wpk_long), witscript['longhash'])
                assert_equal(bytes_to_hex_str(p2wpk_short), witscript['shorthash'])
                mastscript = self.nodes[0].addwitnessaddress(i, bytes_to_hex_str(proof))
                assert_equal(bytes_to_hex_str(p2wpk_mast_long), mastscript['longhash'])
                assert_equal(bytes_to_hex_str(p2wpk_mast_short), mastscript['shorthash'])

        def import_mast_addr(left_addr, left_script, left_known, right_addr, right_script, right_known):
            left_hash = hash256(bytes([0]) + left_script)
            right_hash = hash256(bytes([0]) + right_script)
            p2wpk_mast_long = CScript([OP_0, fastHash256(left_hash, right_hash)])
            try:
                mastscript = self.nodes[0].addwitnessaddress(left_addr, bytes_to_hex_str(bytes([0]) + (bytes([1]) + right_hash)))
                assert_equal(bytes_to_hex_str(p2wpk_mast_long), mastscript['longhash'])
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "Public key or redeemscript not known to wallet, or the key is uncompressed")
                assert_equal(left_known, False)
            try:
                mastscript = self.nodes[0].addwitnessaddress(right_addr, bytes_to_hex_str(bytes([1]) + (bytes([1]) + left_hash)))
                assert_equal(bytes_to_hex_str(p2wpk_mast_long), mastscript['longhash'])
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "Public key or redeemscript not known to wallet, or the key is uncompressed")
                assert_equal(right_known, False)

        # MAST[spendable_p2pk, solvable_p2pk]
        import_mast_addr(spendable_addr, spendable_p2pk, True,
                         solvable_addr, solvable_p2pk, True)
        # MAST[solvable_p2pk, spendable_p2pk]
        import_mast_addr(solvable_addr, solvable_p2pk, True,
                         spendable_addr, spendable_p2pk, True)
        # MAST[spendable_p2pk, unknown_p2pk]
        import_mast_addr(spendable_addr, spendable_p2pk, True,
                         unknown_addr, unknown_p2pk, False)
        # MAST[unknown_p2pk, spendable_p2pk]
        import_mast_addr(unknown_addr, unknown_p2pk, False,
                         spendable_addr, spendable_p2pk, True)
        # MAST[solvable_p2pk, unknown_p2pk]
        import_mast_addr(solvable_addr, solvable_p2pk, True,
                         unknown_addr, unknown_p2pk, False)
        # MAST[unknown_p2pk, solvable_p2pk]
        import_mast_addr(unknown_addr, unknown_p2pk, False,
                         solvable_addr, solvable_p2pk, True)

        spendable_txid.append(self.mine_and_test_listunspent(spendable_after_addwitnessaddress, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_after_addwitnessaddress, 1))
        self.mine_and_test_listunspent(unseen_anytime, 0)

        # Check that spendable outputs are really spendable
        self.create_and_mine_tx_from_txids(spendable_txid)

        # import all the private keys so solvable addresses become spendable
        self.nodes[0].importprivkey("cPiM8Ub4heR9NBYmgVzJQiUH1if44GSBGiqaeJySuL2BKxubvgwb")
        self.nodes[0].importprivkey("cPpAdHaD6VoYbW78kveN2bsvb45Q7G5PhaPApVUGwvF8VQ9brD97")
        self.nodes[0].importprivkey("91zqCU5B9sdWxzMt1ca3VzbtVm2YM6Hi5Rxn4UDtxEaN9C9nzXV")
        self.nodes[0].importprivkey("cPQFjcVRpAUBG8BA9hzr2yEzHwKoMgLkJZBBtK9vJnvGJgMjzTbd")
        self.nodes[0].importprivkey("cQGtcm34xiLjB1v7bkRa4V3aAc9tS2UTuBZ1UnZGeSeNy627fN66")
        self.nodes[0].importprivkey("cTW5mR5M45vHxXkeChZdtSPozrFwFgmEvTNnanCW6wrqwaCZ1X7K")
        self.create_and_mine_tx_from_txids(solvable_txid)

    def mine_and_test_listunspent(self, script_list, ismine):
        utxo = find_unspent(self.nodes[0], 50)
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int('0x'+utxo['txid'],0), utxo['vout'])))
        for i in script_list:
            tx.vout.append(CTxOut(10000000, i))
        tx.rehash()
        signresults = self.nodes[0].signrawtransaction(bytes_to_hex_str(tx.serialize_without_witness()))['hex']
        txid = self.nodes[0].sendrawtransaction(signresults, True)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        watchcount = 0
        spendcount = 0
        for i in self.nodes[0].listunspent():
            if (i['txid'] == txid):
                watchcount += 1
                if (i['spendable'] == True):
                    spendcount += 1
        if (ismine == 2):
            assert_equal(spendcount, len(script_list))
        elif (ismine == 1):
            assert_equal(watchcount, len(script_list))
            assert_equal(spendcount, 0)
        else:
            assert_equal(watchcount, 0)
        return txid

    def p2sh_address_to_script(self,v):
        bare = CScript(hex_str_to_bytes(v['hex']))
        p2sh = CScript(hex_str_to_bytes(v['scriptPubKey']))
        mast = hash256(bytes([0]) + bare)
        p2wsh_long = CScript([OP_0, mast])
        p2wsh_short = CScript([OP_0, ripemd160(mast)])
        skip = hash256(bytes([0]) + CScript([OP_TRUE]))
        proof = bytes([1]) + (bytes([1]) + skip)
        mast = fastHash256(skip, mast)
        p2mast_long = CScript([OP_0, mast])
        p2mast_short = CScript([OP_0, ripemd160(mast)])
        return([bare, p2sh, p2wsh_long, p2wsh_short, p2mast_long, p2mast_short, proof])

    def p2pkh_address_to_script(self,v):
        pubkey = hex_str_to_bytes(v['pubkey'])
        p2pk = CScript([pubkey, OP_CHECKSIG])
        p2sh_p2pk = CScript([OP_HASH160, hash160(p2pk), OP_EQUAL])
        mast = hash256(bytes([0]) + p2pk)
        p2wpk_long = CScript([OP_0, mast])
        p2wpk_short = CScript([OP_0, ripemd160(mast)])
        skip = hash256(bytes([0]) + CScript([OP_TRUE]))
        proof = bytes([1]) + (bytes([1]) + skip)
        mast = fastHash256(skip, mast)
        p2wpk_mast_long = CScript([OP_0, mast])
        p2wpk_mast_short = CScript([OP_0, ripemd160(mast)])
        p2pkh = CScript([OP_DUP, OP_HASH160, hash160(pubkey), OP_EQUALVERIFY, OP_CHECKSIG])
        p2sh_p2pkh = CScript([OP_HASH160, hash160(p2pkh), OP_EQUAL])
        mast = hash256(bytes([0]) + p2pkh)
        p2wpkh_long = CScript([OP_0, mast])
        p2wpkh_short = CScript([OP_0, ripemd160(mast)])
        mast = fastHash256(skip, mast)
        p2wpkh_mast_long = CScript([OP_0, mast])
        p2wpkh_mast_short = CScript([OP_0, ripemd160(mast)])
        return [p2pk, p2sh_p2pk, p2wpk_long, p2wpk_short, p2wpk_mast_long, p2wpk_mast_short, p2pkh, p2sh_p2pkh, p2wpkh_long, p2wpkh_short, p2wpkh_mast_long, p2wpkh_mast_short, proof]

    def create_and_mine_tx_from_txids(self, txids, success = True):
        tx = CTransaction()
        for i in txids:
            txtmp = CTransaction()
            txraw = self.nodes[0].getrawtransaction(i)
            f = BytesIO(hex_str_to_bytes(txraw))
            txtmp.deserialize(f)
            for j in range(len(txtmp.vout)):
                tx.vin.append(CTxIn(COutPoint(int('0x'+i,0), j)))
        tx.vout.append(CTxOut(0, CScript()))
        tx.rehash()
        signresults = self.nodes[0].signrawtransaction(bytes_to_hex_str(tx.serialize_without_witness()))['hex']
        self.nodes[0].sendrawtransaction(signresults, True)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)


if __name__ == '__main__':
    SegWitTest().main()
