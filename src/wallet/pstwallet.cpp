// Copyright (c) 2009-2018 The Bitcoin Core developers
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

#include <wallet/pstwallet.h>

TransactionError FillPST(const CWallet* pwallet, PartiallySignedTransaction& pstx, bool& complete, int sighash_type, bool sign, bool bip32derivs)
{
    LOCK(pwallet->cs_wallet);
    // Get all of the previous transactions
    complete = true;
    for (unsigned int i = 0; i < pstx.tx->vin.size(); ++i) {
        const CTxIn& txin = pstx.tx->vin[i];
        PSTInput& input = pstx.inputs.at(i);

        if (PSTInputSigned(input)) {
            continue;
        }

        // Verify input looks sane. This will check that we have at most one uxto, witness or non-witness.
        if (!input.IsSane()) {
            return TransactionError::INVALID_PST;
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

        complete &= SignPSTInput(HidingSigningProvider(pwallet, !sign, !bip32derivs), pstx, i, sighash_type);
    }

    // Fill in the bip32 keypaths and redeemscripts for the outputs so that hardware wallets can identify change
    for (unsigned int i = 0; i < pstx.tx->vout.size(); ++i) {
        const CTxOut& out = pstx.tx->vout.at(i);
        PSTOutput& pst_out = pstx.outputs.at(i);

        // Fill a SignatureData with output info
        SignatureData sigdata;
        pst_out.FillSignatureData(sigdata);

        MutableTransactionSignatureCreator creator(pstx.tx.get_ptr(), 0, out.GetReferenceValue(), pstx.tx->lock_height, 1);
        ProduceSignature(HidingSigningProvider(pwallet, true, !bip32derivs), creator, out.scriptPubKey, sigdata);
        pst_out.FromSignatureData(sigdata);
    }

    return TransactionError::OK;
}
