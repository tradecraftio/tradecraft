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

#include <amount.h>
#include <coins.h>
#include <consensus/tx_verify.h>
#include <node/pst.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <tinyformat.h>

#include <numeric>

PSTAnalysis AnalyzePST(PartiallySignedTransaction pstx)
{
    // Go through each input and build status
    PSTAnalysis result;

    bool calc_fee = true;
    bool all_final = true;
    bool only_missing_sigs = true;
    bool only_missing_final = false;
    CAmount in_amt = 0;

    result.inputs.resize(pstx.tx->vin.size());

    for (unsigned int i = 0; i < pstx.tx->vin.size(); ++i) {
        PSTInput& input = pstx.inputs[i];
        PSTInputAnalysis& input_analysis = result.inputs[i];

        // Check for a UTXO
        CTxOut utxo;
        uint32_t refheight = 0;
        if (pstx.GetInputUTXO(utxo, refheight, i)) {
            CAmount nInput = utxo.GetTimeAdjustedValue(pstx.tx->lock_height - refheight);
            if (!MoneyRange(utxo.GetReferenceValue()) || !MoneyRange(nInput) || !MoneyRange(in_amt + nInput)) {
                result.SetInvalid(strprintf("PST is not valid. Input %u has invalid value", i));
                return result;
            }
            in_amt += nInput;
            input_analysis.has_utxo = true;
        } else {
            if (input.non_witness_utxo && pstx.tx->vin[i].prevout.n >= input.non_witness_utxo->vout.size()) {
                result.SetInvalid(strprintf("PST is not valid. Input %u specifies invalid prevout", i));
                return result;
            }
            input_analysis.has_utxo = false;
            input_analysis.is_final = false;
            input_analysis.next = PSTRole::UPDATER;
            calc_fee = false;
        }

        if (!utxo.IsNull() && utxo.scriptPubKey.IsUnspendable()) {
            result.SetInvalid(strprintf("PST is not valid. Input %u spends unspendable output", i));
            return result;
        }

        // Check if it is final
        if (!utxo.IsNull() && !PSTInputSigned(input)) {
            input_analysis.is_final = false;
            all_final = false;

            // Figure out what is missing
            SignatureData outdata;
            bool complete = SignPSTInput(DUMMY_SIGNING_PROVIDER, pstx, i, 1, &outdata);

            // Things are missing
            if (!complete) {
                input_analysis.missing_pubkeys = outdata.missing_pubkeys;
                input_analysis.missing_redeem_script = outdata.missing_redeem_script;
                input_analysis.missing_witness_script = outdata.missing_witness_script;
                input_analysis.missing_sigs = outdata.missing_sigs;

                // If we are only missing signatures and nothing else, then next is signer
                if (outdata.missing_pubkeys.empty() && outdata.missing_redeem_script.IsNull() && outdata.missing_witness_script.IsNull() && !outdata.missing_sigs.empty()) {
                    input_analysis.next = PSTRole::SIGNER;
                } else {
                    only_missing_sigs = false;
                    input_analysis.next = PSTRole::UPDATER;
                }
            } else {
                only_missing_final = true;
                input_analysis.next = PSTRole::FINALIZER;
            }
        } else if (!utxo.IsNull()){
            input_analysis.is_final = true;
        }
    }

    if (all_final) {
        only_missing_sigs = false;
        result.next = PSTRole::EXTRACTOR;
    }
    if (calc_fee) {
        // Get the output amount
        CAmount out_amt = std::accumulate(pstx.tx->vout.begin(), pstx.tx->vout.end(), CAmount(0),
            [](CAmount a, const CTxOut& b) {
                if (!MoneyRange(a) || !MoneyRange(b.GetReferenceValue()) || !MoneyRange(a + b.GetReferenceValue())) {
                    return CAmount(-1);
                }
                return a += b.GetReferenceValue();
            }
        );
        if (!MoneyRange(out_amt)) {
            result.SetInvalid(strprintf("PST is not valid. Output amount invalid"));
            return result;
        }

        // Get the fee
        CAmount fee = in_amt - out_amt;
        result.fee = fee;

        // Estimate the size
        CMutableTransaction mtx(*pstx.tx);
        CCoinsView view_dummy;
        CCoinsViewCache view(&view_dummy);
        bool success = true;

        for (unsigned int i = 0; i < pstx.tx->vin.size(); ++i) {
            PSTInput& input = pstx.inputs[i];
            Coin newcoin;

            if (!SignPSTInput(DUMMY_SIGNING_PROVIDER, pstx, i, 1, nullptr, true) || !pstx.GetInputUTXO(newcoin.out, newcoin.refheight, i)) {
                success = false;
                break;
            } else {
                mtx.vin[i].scriptSig = input.final_script_sig;
                mtx.vin[i].scriptWitness = input.final_script_witness;
                newcoin.nHeight = 1;
                view.AddCoin(pstx.tx->vin[i].prevout, std::move(newcoin), true);
            }
        }

        if (success) {
            CTransaction ctx = CTransaction(mtx);
            size_t size = GetVirtualTransactionSize(ctx, GetTransactionSigOpCost(ctx, view, STANDARD_SCRIPT_VERIFY_FLAGS));
            result.estimated_vsize = size;
            // Estimate fee rate
            CFeeRate feerate(fee, size);
            result.estimated_feerate = feerate;
        }

        if (only_missing_sigs) {
            result.next = PSTRole::SIGNER;
        } else if (only_missing_final) {
            result.next = PSTRole::FINALIZER;
        } else if (all_final) {
            result.next = PSTRole::EXTRACTOR;
        } else {
            result.next = PSTRole::UPDATER;
        }
    } else {
        result.next = PSTRole::UPDATER;
    }

    return result;
}
