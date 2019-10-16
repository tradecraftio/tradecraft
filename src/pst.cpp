// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
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

#include <pst.h>

#include <util/check.h>
#include <util/strencodings.h>


PartiallySignedTransaction::PartiallySignedTransaction(const CMutableTransaction& tx) : tx(tx)
{
    inputs.resize(tx.vin.size());
    outputs.resize(tx.vout.size());
}

bool PartiallySignedTransaction::IsNull() const
{
    return !tx && inputs.empty() && outputs.empty() && unknown.empty();
}

bool PartiallySignedTransaction::Merge(const PartiallySignedTransaction& pst)
{
    // Prohibited to merge two PSTs over different transactions
    if (tx->GetHash() != pst.tx->GetHash()) {
        return false;
    }

    for (unsigned int i = 0; i < inputs.size(); ++i) {
        inputs[i].Merge(pst.inputs[i]);
    }
    for (unsigned int i = 0; i < outputs.size(); ++i) {
        outputs[i].Merge(pst.outputs[i]);
    }
    for (auto& xpub_pair : pst.m_xpubs) {
        if (m_xpubs.count(xpub_pair.first) == 0) {
            m_xpubs[xpub_pair.first] = xpub_pair.second;
        } else {
            m_xpubs[xpub_pair.first].insert(xpub_pair.second.begin(), xpub_pair.second.end());
        }
    }
    unknown.insert(pst.unknown.begin(), pst.unknown.end());

    return true;
}

bool PartiallySignedTransaction::AddInput(const CTxIn& txin, PSTInput& pstin)
{
    if (std::find(tx->vin.begin(), tx->vin.end(), txin) != tx->vin.end()) {
        return false;
    }
    tx->vin.push_back(txin);
    pstin.partial_sigs.clear();
    pstin.final_script_sig.clear();
    pstin.final_script_witness.SetNull();
    inputs.push_back(pstin);
    return true;
}

bool PartiallySignedTransaction::AddOutput(const CTxOut& txout, const PSTOutput& pstout)
{
    tx->vout.push_back(txout);
    outputs.push_back(pstout);
    return true;
}

bool PartiallySignedTransaction::GetInputUTXO(SpentOutput& utxo, int input_index) const
{
    const PSTInput& input = inputs[input_index];
    uint32_t prevout_index = tx->vin[input_index].prevout.n;
    if (input.non_witness_utxo) {
        if (prevout_index >= input.non_witness_utxo->vout.size()) {
            return false;
        }
        if (input.non_witness_utxo->GetHash() != tx->vin[input_index].prevout.hash) {
            return false;
        }
        utxo = {input.non_witness_utxo->vout[prevout_index], input.non_witness_utxo->lock_height};
    } else if (!input.witness_utxo.IsNull()) {
        utxo = {input.witness_utxo, input.witness_refheight};
    } else {
        return false;
    }
    return true;
}

bool PSTInput::IsNull() const
{
    return !non_witness_utxo && witness_utxo.IsNull() && partial_sigs.empty() && unknown.empty() && hd_keypaths.empty() && redeem_script.empty() && witness_entry.IsNull();
}

void PSTInput::FillSignatureData(SignatureData& sigdata) const
{
    if (!final_script_sig.empty()) {
        sigdata.scriptSig = final_script_sig;
        sigdata.complete = true;
    }
    if (!final_script_witness.IsNull()) {
        sigdata.scriptWitness = final_script_witness;
        sigdata.complete = true;
    }
    if (sigdata.complete) {
        return;
    }

    sigdata.signatures.insert(partial_sigs.begin(), partial_sigs.end());
    if (!redeem_script.empty()) {
        sigdata.redeem_script = redeem_script;
    }
    if (!witness_entry.IsNull()) {
        sigdata.witness_entry = witness_entry;
    }
    for (const auto& key_pair : hd_keypaths) {
        sigdata.misc_pubkeys.emplace(key_pair.first.GetID(), key_pair);
    }
}

void PSTInput::FromSignatureData(const SignatureData& sigdata)
{
    if (sigdata.complete) {
        partial_sigs.clear();
        hd_keypaths.clear();
        redeem_script.clear();
        witness_entry.SetNull();

        if (!sigdata.scriptSig.empty()) {
            final_script_sig = sigdata.scriptSig;
        }
        if (!sigdata.scriptWitness.IsNull()) {
            final_script_witness = sigdata.scriptWitness;
        }
        return;
    }

    partial_sigs.insert(sigdata.signatures.begin(), sigdata.signatures.end());
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    if (witness_entry.IsNull() && !sigdata.witness_entry.IsNull()) {
        witness_entry = sigdata.witness_entry;
    }
    for (const auto& entry : sigdata.misc_pubkeys) {
        hd_keypaths.emplace(entry.second);
    }
}

void PSTInput::Merge(const PSTInput& input)
{
    if (!non_witness_utxo && input.non_witness_utxo) non_witness_utxo = input.non_witness_utxo;
    if (witness_utxo.IsNull() && !input.witness_utxo.IsNull()) {
        // TODO: For segwit v1, we will want to clear out the non-witness utxo when setting a witness one. For v0 and non-segwit, this is not safe
        witness_utxo = input.witness_utxo;
        witness_refheight = input.witness_refheight;
    }

    partial_sigs.insert(input.partial_sigs.begin(), input.partial_sigs.end());
    ripemd160_preimages.insert(input.ripemd160_preimages.begin(), input.ripemd160_preimages.end());
    sha256_preimages.insert(input.sha256_preimages.begin(), input.sha256_preimages.end());
    hash160_preimages.insert(input.hash160_preimages.begin(), input.hash160_preimages.end());
    hash256_preimages.insert(input.hash256_preimages.begin(), input.hash256_preimages.end());
    hd_keypaths.insert(input.hd_keypaths.begin(), input.hd_keypaths.end());
    unknown.insert(input.unknown.begin(), input.unknown.end());

    if (redeem_script.empty() && !input.redeem_script.empty()) redeem_script = input.redeem_script;
    if (witness_entry.IsNull() && !input.witness_entry.IsNull()) witness_entry = input.witness_entry;
    if (final_script_sig.empty() && !input.final_script_sig.empty()) final_script_sig = input.final_script_sig;
    if (final_script_witness.IsNull() && !input.final_script_witness.IsNull()) final_script_witness = input.final_script_witness;
}

void PSTOutput::FillSignatureData(SignatureData& sigdata) const
{
    if (!redeem_script.empty()) {
        sigdata.redeem_script = redeem_script;
    }
    if (!witness_entry.IsNull()) {
        sigdata.witness_entry = witness_entry;
    }
    for (const auto& key_pair : hd_keypaths) {
        sigdata.misc_pubkeys.emplace(key_pair.first.GetID(), key_pair);
    }
}

void PSTOutput::FromSignatureData(const SignatureData& sigdata)
{
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    if (witness_entry.IsNull() && !sigdata.witness_entry.IsNull()) {
        witness_entry = sigdata.witness_entry;
    }
    for (const auto& entry : sigdata.misc_pubkeys) {
        hd_keypaths.emplace(entry.second);
    }
}

bool PSTOutput::IsNull() const
{
    return redeem_script.empty() && witness_entry.IsNull() && hd_keypaths.empty() && unknown.empty();
}

void PSTOutput::Merge(const PSTOutput& output)
{
    hd_keypaths.insert(output.hd_keypaths.begin(), output.hd_keypaths.end());
    unknown.insert(output.unknown.begin(), output.unknown.end());

    if (redeem_script.empty() && !output.redeem_script.empty()) redeem_script = output.redeem_script;
    if (witness_entry.IsNull() && !output.witness_entry.IsNull()) witness_entry = output.witness_entry;
}
bool PSTInputSigned(const PSTInput& input)
{
    return !input.final_script_sig.empty() || !input.final_script_witness.IsNull();
}

size_t CountPSTUnsignedInputs(const PartiallySignedTransaction& pst) {
    size_t count = 0;
    for (const auto& input : pst.inputs) {
        if (!PSTInputSigned(input)) {
            count++;
        }
    }

    return count;
}

void UpdatePSTOutput(const SigningProvider& provider, PartiallySignedTransaction& pst, int index)
{
    CMutableTransaction& tx = *Assert(pst.tx);
    const CTxOut& out = tx.vout.at(index);
    PSTOutput& pst_out = pst.outputs.at(index);

    // Fill a SignatureData with output info
    SignatureData sigdata;
    pst_out.FillSignatureData(sigdata);

    // Construct a would-be spend of this output, to update sigdata with.
    // Note that ProduceSignature is used to fill in metadata (not actual signatures),
    // so provider does not need to provide any private keys (it can be a HidingSigningProvider).
    MutableTransactionSignatureCreator creator(&tx, /*input_idx=*/0, out.GetReferenceValue(), pst.tx->lock_height, SIGHASH_ALL);
    ProduceSignature(provider, creator, out.scriptPubKey, sigdata);

    // Put redeem_script, witness_entry, key paths, into PSTOutput.
    pst_out.FromSignatureData(sigdata);
}

PrecomputedTransactionData PrecomputePSTData(const PartiallySignedTransaction& pst)
{
    const CMutableTransaction& tx = *pst.tx;
    bool have_all_spent_outputs = true;
    std::vector<SpentOutput> utxos(tx.vin.size());
    for (size_t idx = 0; idx < tx.vin.size(); ++idx) {
        if (!pst.GetInputUTXO(utxos[idx], idx)) have_all_spent_outputs = false;
    }
    PrecomputedTransactionData txdata;
    if (have_all_spent_outputs) {
        txdata.Init(tx, std::move(utxos), true);
    } else {
        txdata.Init(tx, {}, true);
    }
    return txdata;
}

bool SignPSTInput(const SigningProvider& provider, PartiallySignedTransaction& pst, int index, const PrecomputedTransactionData* txdata, int sighash,  SignatureData* out_sigdata, bool finalize)
{
    PSTInput& input = pst.inputs.at(index);
    const CMutableTransaction& tx = *pst.tx;

    if (PSTInputSigned(input)) {
        return true;
    }

    // Fill SignatureData with input info
    SignatureData sigdata;
    input.FillSignatureData(sigdata);

    // Get UTXO
    bool require_witness_sig = false;
    CTxOut utxo;
    uint32_t refheight;

    if (input.non_witness_utxo) {
        // If we're taking our information from a non-witness UTXO, verify that it matches the prevout.
        COutPoint prevout = tx.vin[index].prevout;
        if (prevout.n >= input.non_witness_utxo->vout.size()) {
            return false;
        }
        if (input.non_witness_utxo->GetHash() != prevout.hash) {
            return false;
        }
        utxo = input.non_witness_utxo->vout[prevout.n];
        refheight = input.non_witness_utxo->lock_height;
    } else if (!input.witness_utxo.IsNull()) {
        utxo = input.witness_utxo;
        refheight = input.witness_refheight;
        // When we're taking our information from a witness UTXO, we can't verify it is actually data from
        // the output being spent. This is safe in case a witness signature is produced (which includes this
        // information directly in the hash), but not for non-witness signatures. Remember that we require
        // a witness signature in this situation.
        require_witness_sig = true;
    } else {
        return false;
    }

    sigdata.witness = false;
    bool sig_complete;
    if (txdata == nullptr) {
        sig_complete = ProduceSignature(provider, DUMMY_SIGNATURE_CREATOR, utxo.scriptPubKey, sigdata);
    } else {
        MutableTransactionSignatureCreator creator(&tx, index, utxo.GetReferenceValue(), refheight, txdata, sighash);
        sig_complete = ProduceSignature(provider, creator, utxo.scriptPubKey, sigdata);
    }
    // Verify that a witness signature was produced in case one was required.
    if (require_witness_sig && !sigdata.witness) return false;

    // If we are not finalizing, set sigdata.complete to false to not set the scriptWitness
    if (!finalize && sigdata.complete) sigdata.complete = false;

    input.FromSignatureData(sigdata);

    // If we have a witness signature, put a witness UTXO.
    // TODO: For segwit v1, we should remove the non_witness_utxo
    if (sigdata.witness) {
        input.witness_utxo = utxo;
        input.witness_refheight = refheight;
        // input.non_witness_utxo = nullptr;
    }

    // Fill in the missing info
    if (out_sigdata) {
        out_sigdata->missing_pubkeys = sigdata.missing_pubkeys;
        out_sigdata->missing_sigs = sigdata.missing_sigs;
        out_sigdata->missing_redeem_script = sigdata.missing_redeem_script;
        out_sigdata->missing_witness_script = sigdata.missing_witness_script;
    }

    return sig_complete;
}

bool FinalizePST(PartiallySignedTransaction& pstx)
{
    // Finalize input signatures -- in case we have partial signatures that add up to a complete
    //   signature, but have not combined them yet (e.g. because the combiner that created this
    //   PartiallySignedTransaction did not understand them), this will combine them into a final
    //   script.
    bool complete = true;
    const PrecomputedTransactionData txdata = PrecomputePSTData(pstx);
    for (unsigned int i = 0; i < pstx.tx->vin.size(); ++i) {
        complete &= SignPSTInput(DUMMY_SIGNING_PROVIDER, pstx, i, &txdata, SIGHASH_ALL, nullptr, true);
    }

    return complete;
}

bool FinalizeAndExtractPST(PartiallySignedTransaction& pstx, CMutableTransaction& result)
{
    // It's not safe to extract a PST that isn't finalized, and there's no easy way to check
    //   whether a PST is finalized without finalizing it, so we just do this.
    if (!FinalizePST(pstx)) {
        return false;
    }

    result = *pstx.tx;
    for (unsigned int i = 0; i < result.vin.size(); ++i) {
        result.vin[i].scriptSig = pstx.inputs[i].final_script_sig;
        result.vin[i].scriptWitness = pstx.inputs[i].final_script_witness;
    }
    return true;
}

TransactionError CombinePSTs(PartiallySignedTransaction& out, const std::vector<PartiallySignedTransaction>& pstxs)
{
    out = pstxs[0]; // Copy the first one

    // Merge
    for (auto it = std::next(pstxs.begin()); it != pstxs.end(); ++it) {
        if (!out.Merge(*it)) {
            return TransactionError::PST_MISMATCH;
        }
    }
    return TransactionError::OK;
}

std::string PSTRoleName(PSTRole role) {
    switch (role) {
    case PSTRole::CREATOR: return "creator";
    case PSTRole::UPDATER: return "updater";
    case PSTRole::SIGNER: return "signer";
    case PSTRole::FINALIZER: return "finalizer";
    case PSTRole::EXTRACTOR: return "extractor";
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

bool DecodeHexPST(PartiallySignedTransaction& pst, const std::string& hex_pst, std::string& error)
{
    if (!IsHex(hex_pst)) {
        error = "invalid hex";
        return false;
    }
    return DecodeRawPST(pst, ParseHex(hex_pst), error);
}

bool DecodeRawPST(PartiallySignedTransaction& pst, const std::vector<unsigned char>& tx_data, std::string& error)
{
    CDataStream ss_data(MakeByteSpan(tx_data), SER_NETWORK, PROTOCOL_VERSION);
    try {
        ss_data >> pst;
        if (!ss_data.empty()) {
            error = "extra data after PST";
            return false;
        }
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
    return true;
}

uint32_t PartiallySignedTransaction::GetVersion() const
{
    if (m_version != std::nullopt) {
        return *m_version;
    }
    return 0;
}
