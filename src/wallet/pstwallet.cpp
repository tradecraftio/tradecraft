// Copyright (c) 2009-2019 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <wallet/psbtwallet.h>

TransactionError FillPSBT(const CWallet* pwallet, PartiallySignedTransaction& psbtx, bool& complete, int sighash_type, bool sign, bool bip32derivs)
{
    LOCK(pwallet->cs_wallet);
    // Get all of the previous transactions
    complete = true;
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Verify input looks sane. This will check that we have at most one uxto, witness or non-witness.
        if (!input.IsSane()) {
            return TransactionError::INVALID_PSBT;
        }

        // If we have no utxo, grab it from the wallet.
        if (!input.non_witness_utxo && input.witness_utxo.IsNull()) {
            const uint256& txhash = txin.prevout.hash;
            const auto it = pwallet->mapWallet.find(txhash);
            if (it != pwallet->mapWallet.end()) {
                const CWalletTx& wtx = it->second;
                // We only need the non_witness_utxo, which is a superset of the witness_utxo.
                //   The signing code will switch to the smaller witness_utxo if this is ok.
                input.non_witness_utxo = wtx.tx;
            }
        }

        // Get the Sighash type
        if (sign && input.sighash_type > 0 && input.sighash_type != sighash_type) {
            return TransactionError::SIGHASH_MISMATCH;
        }

        // Backport of #17156 fix, without #17371 refactor
        if (input.witness_utxo.IsNull() && input.non_witness_utxo) {
            if (txin.prevout.n >= input.non_witness_utxo->vout.size()) {
                return TransactionError::MISSING_INPUTS;
            }
        }

        complete &= SignPSBTInput(HidingSigningProvider(pwallet, !sign, !bip32derivs), psbtx, i, sighash_type);
    }

    // Fill in the bip32 keypaths and redeemscripts for the outputs so that hardware wallets can identify change
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        UpdatePSBTOutput(HidingSigningProvider(pwallet, true, !bip32derivs), psbtx, i);
    }

    return TransactionError::OK;
}
