#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Copyright (c) 2010-2024 The Freicoin Developers
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
"""Test a basic M-of-N multisig setup between multiple people using descriptor wallets and PSTs, as well as a signing flow.

This is meant to be documentation as much as functional tests, so it is kept as simple and readable as possible.
"""

from test_framework.address import base58_to_byte
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
)


class WalletMultisigDescriptorPSTTest(FreicoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        self.wallet_names = []
        self.extra_args = [["-keypool=100"]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    @staticmethod
    def _get_xpub(wallet):
        """Extract the wallet's xpubs using `listdescriptors` and pick the one from the `pkh` descriptor since it's least likely to be accidentally reused (legacy addresses)."""
        descriptor = next(filter(lambda d: d["desc"].startswith("pkh"), wallet.listdescriptors()["descriptors"]))
        return descriptor["desc"].split("]")[-1].split("/")[0]

    @staticmethod
    def _check_pst(pst, to, value, multisig):
        """Helper function for any of the N participants to check the pst with decodepst and verify it is OK before signing."""
        tx = multisig.decodepst(pst)["tx"]
        amount = 0
        for vout in tx["vout"]:
            address = vout["scriptPubKey"]["address"]
            assert_equal(multisig.getaddressinfo(address)["ischange"], address != to)
            if address == to:
                amount += vout["value"]
        assert_approx(amount, float(value), vspan=0.001)

    def participants_create_multisigs(self, xpubs):
        """The multisig is created by importing the following descriptors. The resulting wallet is watch-only and every participant can do this."""
        # some simple validation
        assert_equal(len(xpubs), self.N)
        # a sanity-check/assertion, this will throw if the base58 checksum of any of the provided xpubs are invalid
        for xpub in xpubs:
            base58_to_byte(xpub)

        for i, node in enumerate(self.nodes):
            node.createwallet(wallet_name=f"{self.name}_{i}", blank=True, descriptors=True, disable_private_keys=True)
            multisig = node.get_wallet_rpc(f"{self.name}_{i}")
            external = multisig.getdescriptorinfo(f"wsh(sortedmulti({self.M},{f'/0/*,'.join(xpubs)}/0/*))")
            internal = multisig.getdescriptorinfo(f"wsh(sortedmulti({self.M},{f'/1/*,'.join(xpubs)}/1/*))")
            result = multisig.importdescriptors([
                {  # receiving addresses (internal: False)
                    "desc": external["descriptor"],
                    "active": True,
                    "internal": False,
                    "timestamp": "now",
                },
                {  # change addresses (internal: True)
                    "desc": internal["descriptor"],
                    "active": True,
                    "internal": True,
                    "timestamp": "now",
                },
            ])
            assert all(r["success"] for r in result)
            yield multisig

    def run_test(self):
        self.M = 2
        self.N = self.num_nodes
        self.name = f"{self.M}_of_{self.N}_multisig"
        self.log.info(f"Testing {self.name}...")

        participants = {
            # Every participant generates an xpub. The most straightforward way is to create a new descriptor wallet.
            # This wallet will be the participant's `signer` for the resulting multisig. Avoid reusing this wallet for any other purpose (for privacy reasons).
            "signers": [node.get_wallet_rpc(node.createwallet(wallet_name=f"participant_{self.nodes.index(node)}", descriptors=True)["name"]) for node in self.nodes],
            # After participants generate and exchange their xpubs they will each create their own watch-only multisig.
            # Note: these multisigs are all the same, this just highlights that each participant can independently verify everything on their own node.
            "multisigs": []
        }

        self.log.info("Generate and exchange xpubs...")
        xpubs = [self._get_xpub(signer) for signer in participants["signers"]]

        self.log.info("Every participant imports the following descriptors to create the watch-only multisig...")
        participants["multisigs"] = list(self.participants_create_multisigs(xpubs))

        self.log.info("Check that every participant's multisig generates the same addresses...")
        for _ in range(10):  # we check that the first 10 generated addresses are the same for all participant's multisigs
            receive_addresses = [multisig.getnewaddress() for multisig in participants["multisigs"]]
            all(address == receive_addresses[0] for address in receive_addresses)
            change_addresses = [multisig.getrawchangeaddress() for multisig in participants["multisigs"]]
            all(address == change_addresses[0] for address in change_addresses)

        self.log.info("Get a mature utxo to send to the multisig...")
        coordinator_wallet = participants["signers"][0]
        self.generatetoaddress(self.nodes[0], 101, coordinator_wallet.getnewaddress())

        deposit_amount = 6.15
        multisig_receiving_address = participants["multisigs"][0].getnewaddress()
        self.log.info("Send funds to the resulting multisig receiving address...")
        coordinator_wallet.sendtoaddress(multisig_receiving_address, deposit_amount)
        self.generate(self.nodes[0], 1)
        for participant in participants["multisigs"]:
            assert_approx(participant.getbalance(), deposit_amount, vspan=0.001)

        self.log.info("Send a transaction from the multisig!")
        to = participants["signers"][self.N - 1].getnewaddress()
        value = 1
        self.log.info("First, make a sending transaction, created using `walletcreatefundedpst` (anyone can initiate this)...")
        pst = participants["multisigs"][0].walletcreatefundedpst(inputs=[], outputs={to: value}, feeRate=0.00010)

        psts = []
        self.log.info("Now at least M users check the pst with decodepst and (if OK) signs it with walletprocesspst...")
        for m in range(self.M):
            signers_multisig = participants["multisigs"][m]
            self._check_pst(pst["pst"], to, value, signers_multisig)
            signing_wallet = participants["signers"][m]
            partially_signed_pst = signing_wallet.walletprocesspst(pst["pst"])
            psts.append(partially_signed_pst["pst"])

        self.log.info("Finally, collect the signed PSTs with combinepst, finalizepst, then broadcast the resulting transaction...")
        combined = coordinator_wallet.combinepst(psts)
        finalized = coordinator_wallet.finalizepst(combined)
        coordinator_wallet.sendrawtransaction(finalized["hex"])

        self.log.info("Check that balances are correct after the transaction has been included in a block.")
        self.generate(self.nodes[0], 1)
        assert_approx(participants["multisigs"][0].getbalance(), deposit_amount - value, vspan=0.001)
        assert_equal(participants["signers"][self.N - 1].getbalance(), value)

        self.log.info("Send another transaction from the multisig, this time with a daisy chained signing flow (one after another in series)!")
        pst = participants["multisigs"][0].walletcreatefundedpst(inputs=[], outputs={to: value}, feeRate=0.00010)
        for m in range(self.M):
            signers_multisig = participants["multisigs"][m]
            self._check_pst(pst["pst"], to, value, signers_multisig)
            signing_wallet = participants["signers"][m]
            pst = signing_wallet.walletprocesspst(pst["pst"])
            assert_equal(pst["complete"], m == self.M - 1)
        coordinator_wallet.sendrawtransaction(pst["hex"])

        self.log.info("Check that balances are correct after the transaction has been included in a block.")
        self.generate(self.nodes[0], 1)
        assert_approx(participants["multisigs"][0].getbalance(), deposit_amount - (value * 2), vspan=0.001)
        assert_equal(participants["signers"][self.N - 1].getbalance(), value * 2)


if __name__ == "__main__":
    WalletMultisigDescriptorPSTTest().main()
