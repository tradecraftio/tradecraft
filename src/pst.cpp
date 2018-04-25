// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#include <policy/policy.h>
#include <script/signingprovider.h>
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
    return !non_witness_utxo && witness_utxo.IsNull() && partial_sigs.empty() && unknown.empty() && hd_keypaths.empty() && redeem_script.empty() && witness_script.empty();
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
    if (!witness_script.empty()) {
        sigdata.witness_script = witness_script;
    }
    for (const auto& key_pair : hd_keypaths) {
        sigdata.misc_pubkeys.emplace(key_pair.first.GetID(), key_pair);
    }
    if (!m_tap_key_sig.empty()) {
        sigdata.taproot_key_path_sig = m_tap_key_sig;
    }
    for (const auto& [pubkey_leaf, sig] : m_tap_script_sigs) {
        sigdata.taproot_script_sigs.emplace(pubkey_leaf, sig);
    }
    if (!m_tap_internal_key.IsNull()) {
        sigdata.tr_spenddata.internal_key = m_tap_internal_key;
    }
    if (!m_tap_merkle_root.IsNull()) {
        sigdata.tr_spenddata.merkle_root = m_tap_merkle_root;
    }
    for (const auto& [leaf_script, control_block] : m_tap_scripts) {
        sigdata.tr_spenddata.scripts.emplace(leaf_script, control_block);
    }
    for (const auto& [pubkey, leaf_origin] : m_tap_bip32_paths) {
        sigdata.taproot_misc_pubkeys.emplace(pubkey, leaf_origin);
        sigdata.tap_pubkeys.emplace(Hash160(pubkey), pubkey);
    }
    for (const auto& [hash, preimage] : ripemd160_preimages) {
        sigdata.ripemd160_preimages.emplace(std::vector<unsigned char>(hash.begin(), hash.end()), preimage);
    }
    for (const auto& [hash, preimage] : sha256_preimages) {
        sigdata.sha256_preimages.emplace(std::vector<unsigned char>(hash.begin(), hash.end()), preimage);
    }
    for (const auto& [hash, preimage] : hash160_preimages) {
        sigdata.hash160_preimages.emplace(std::vector<unsigned char>(hash.begin(), hash.end()), preimage);
    }
    for (const auto& [hash, preimage] : hash256_preimages) {
        sigdata.hash256_preimages.emplace(std::vector<unsigned char>(hash.begin(), hash.end()), preimage);
    }
}

void PSTInput::FromSignatureData(const SignatureData& sigdata)
{
    if (sigdata.complete) {
        partial_sigs.clear();
        hd_keypaths.clear();
        redeem_script.clear();
        witness_script.clear();

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
    if (witness_script.empty() && !sigdata.witness_script.empty()) {
        witness_script = sigdata.witness_script;
    }
    for (const auto& entry : sigdata.misc_pubkeys) {
        hd_keypaths.emplace(entry.second);
    }
    if (!sigdata.taproot_key_path_sig.empty()) {
        m_tap_key_sig = sigdata.taproot_key_path_sig;
    }
    for (const auto& [pubkey_leaf, sig] : sigdata.taproot_script_sigs) {
        m_tap_script_sigs.emplace(pubkey_leaf, sig);
    }
    if (!sigdata.tr_spenddata.internal_key.IsNull()) {
        m_tap_internal_key = sigdata.tr_spenddata.internal_key;
    }
    if (!sigdata.tr_spenddata.merkle_root.IsNull()) {
        m_tap_merkle_root = sigdata.tr_spenddata.merkle_root;
    }
    for (const auto& [leaf_script, control_block] : sigdata.tr_spenddata.scripts) {
        m_tap_scripts.emplace(leaf_script, control_block);
    }
    for (const auto& [pubkey, leaf_origin] : sigdata.taproot_misc_pubkeys) {
        m_tap_bip32_paths.emplace(pubkey, leaf_origin);
    }
}

void PSTInput::Merge(const PSTInput& input)
{
    if (!non_witness_utxo && input.non_witness_utxo) non_witness_utxo = input.non_witness_utxo;
    if (witness_utxo.IsNull() && !input.witness_utxo.IsNull()) {
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
    m_tap_script_sigs.insert(input.m_tap_script_sigs.begin(), input.m_tap_script_sigs.end());
    m_tap_scripts.insert(input.m_tap_scripts.begin(), input.m_tap_scripts.end());
    m_tap_bip32_paths.insert(input.m_tap_bip32_paths.begin(), input.m_tap_bip32_paths.end());

    if (redeem_script.empty() && !input.redeem_script.empty()) redeem_script = input.redeem_script;
    if (witness_script.empty() && !input.witness_script.empty()) witness_script = input.witness_script;
    if (final_script_sig.empty() && !input.final_script_sig.empty()) final_script_sig = input.final_script_sig;
    if (final_script_witness.IsNull() && !input.final_script_witness.IsNull()) final_script_witness = input.final_script_witness;
    if (m_tap_key_sig.empty() && !input.m_tap_key_sig.empty()) m_tap_key_sig = input.m_tap_key_sig;
    if (m_tap_internal_key.IsNull() && !input.m_tap_internal_key.IsNull()) m_tap_internal_key = input.m_tap_internal_key;
    if (m_tap_merkle_root.IsNull() && !input.m_tap_merkle_root.IsNull()) m_tap_merkle_root = input.m_tap_merkle_root;
}

void PSTOutput::FillSignatureData(SignatureData& sigdata) const
{
    if (!redeem_script.empty()) {
        sigdata.redeem_script = redeem_script;
    }
    if (!witness_script.empty()) {
        sigdata.witness_script = witness_script;
    }
    for (const auto& key_pair : hd_keypaths) {
        sigdata.misc_pubkeys.emplace(key_pair.first.GetID(), key_pair);
    }
    if (!m_tap_tree.empty() && m_tap_internal_key.IsFullyValid()) {
        TaprootBuilder builder;
        for (const auto& [depth, leaf_ver, script] : m_tap_tree) {
            builder.Add((int)depth, script, (int)leaf_ver, /*track=*/true);
        }
        assert(builder.IsComplete());
        builder.Finalize(m_tap_internal_key);
        TaprootSpendData spenddata = builder.GetSpendData();

        sigdata.tr_spenddata.internal_key = m_tap_internal_key;
        sigdata.tr_spenddata.Merge(spenddata);
    }
    for (const auto& [pubkey, leaf_origin] : m_tap_bip32_paths) {
        sigdata.taproot_misc_pubkeys.emplace(pubkey, leaf_origin);
        sigdata.tap_pubkeys.emplace(Hash160(pubkey), pubkey);
    }
}

void PSTOutput::FromSignatureData(const SignatureData& sigdata)
{
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    if (witness_script.empty() && !sigdata.witness_script.empty()) {
        witness_script = sigdata.witness_script;
    }
    for (const auto& entry : sigdata.misc_pubkeys) {
        hd_keypaths.emplace(entry.second);
    }
    if (!sigdata.tr_spenddata.internal_key.IsNull()) {
        m_tap_internal_key = sigdata.tr_spenddata.internal_key;
    }
    if (sigdata.tr_builder.has_value() && sigdata.tr_builder->HasScripts()) {
        m_tap_tree = sigdata.tr_builder->GetTreeTuples();
    }
    for (const auto& [pubkey, leaf_origin] : sigdata.taproot_misc_pubkeys) {
        m_tap_bip32_paths.emplace(pubkey, leaf_origin);
    }
}

bool PSTOutput::IsNull() const
{
    return redeem_script.empty() && witness_script.empty() && hd_keypaths.empty() && unknown.empty();
}

void PSTOutput::Merge(const PSTOutput& output)
{
    hd_keypaths.insert(output.hd_keypaths.begin(), output.hd_keypaths.end());
    unknown.insert(output.unknown.begin(), output.unknown.end());
    m_tap_bip32_paths.insert(output.m_tap_bip32_paths.begin(), output.m_tap_bip32_paths.end());

    if (redeem_script.empty() && !output.redeem_script.empty()) redeem_script = output.redeem_script;
    if (witness_script.empty() && !output.witness_script.empty()) witness_script = output.witness_script;
    if (m_tap_internal_key.IsNull() && !output.m_tap_internal_key.IsNull()) m_tap_internal_key = output.m_tap_internal_key;
    if (m_tap_tree.empty() && !output.m_tap_tree.empty()) m_tap_tree = output.m_tap_tree;
}

bool PSTInputSigned(const PSTInput& input)
{
    return !input.final_script_sig.empty() || !input.final_script_witness.IsNull();
}

bool PSTInputSignedAndVerified(const PartiallySignedTransaction pst, unsigned int input_index, const PrecomputedTransactionData* txdata)
{
    SpentOutput spent_output;
    assert(pst.inputs.size() >= input_index);
    const PSTInput& input = pst.inputs[input_index];

    if (input.non_witness_utxo) {
        // If we're taking our information from a non-witness UTXO, verify that it matches the prevout.
        COutPoint prevout = pst.tx->vin[input_index].prevout;
        if (prevout.n >= input.non_witness_utxo->vout.size()) {
            return false;
        }
        if (input.non_witness_utxo->GetHash() != prevout.hash) {
            return false;
        }
        spent_output.out = input.non_witness_utxo->vout[prevout.n];
        spent_output.refheight = input.non_witness_utxo->lock_height;
    } else if (!input.witness_utxo.IsNull()) {
        spent_output.out = input.witness_utxo;
        spent_output.refheight = input.witness_refheight;
    } else {
        return false;
    }

    if (txdata) {
        return VerifyScript(input.final_script_sig, spent_output.out.scriptPubKey, &input.final_script_witness, STANDARD_SCRIPT_VERIFY_FLAGS, MutableTransactionSignatureChecker{&(*pst.tx), input_index, spent_output.out.GetReferenceValue(), spent_output.refheight, *txdata, MissingDataBehavior::FAIL});
    } else {
        return VerifyScript(input.final_script_sig, spent_output.out.scriptPubKey, &input.final_script_witness, STANDARD_SCRIPT_VERIFY_FLAGS, MutableTransactionSignatureChecker{&(*pst.tx), input_index, spent_output.out.GetReferenceValue(), spent_output.refheight, MissingDataBehavior::FAIL});
    }
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
    MutableTransactionSignatureCreator creator(tx, /*input_idx=*/0, out.GetReferenceValue(), pst.tx->lock_height, SIGHASH_ALL);
    ProduceSignature(provider, creator, out.scriptPubKey, sigdata);

    // Put redeem_script, witness_script, key paths, into PSTOutput.
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

    if (PSTInputSignedAndVerified(pst, index, txdata)) {
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
        MutableTransactionSignatureCreator creator(tx, index, utxo.GetReferenceValue(), refheight, txdata, sighash);
        sig_complete = ProduceSignature(provider, creator, utxo.scriptPubKey, sigdata);
    }
    // Verify that a witness signature was produced in case one was required.
    if (require_witness_sig && !sigdata.witness) return false;

    // If we are not finalizing, set sigdata.complete to false to not set the scriptWitness
    if (!finalize && sigdata.complete) sigdata.complete = false;

    input.FromSignatureData(sigdata);

    // If we have a witness signature, put a witness UTXO.
    if (sigdata.witness) {
        input.witness_utxo = utxo;
        input.witness_refheight = refheight;
        // We can remove the non_witness_utxo if and only if there are no non-segwit or segwit v0
        // inputs in this transaction. Since this requires inspecting the entire transaction, this
        // is something for the caller to deal with (i.e. FillPST).
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

void RemoveUnnecessaryTransactions(PartiallySignedTransaction& pstx, const int& sighash_type)
{
    // Only drop non_witness_utxos if sighash_type != SIGHASH_ANYONECANPAY
    if ((sighash_type & 0x80) != SIGHASH_ANYONECANPAY) {
        // Figure out if any non_witness_utxos should be dropped
        std::vector<unsigned int> to_drop;
        for (unsigned int i = 0; i < pstx.inputs.size(); ++i) {
            const auto& input = pstx.inputs.at(i);
            int wit_ver;
            std::vector<unsigned char> wit_prog;
            if (input.witness_utxo.IsNull() || !input.witness_utxo.scriptPubKey.IsWitnessProgram(wit_ver, wit_prog)) {
                // There's a non-segwit input or Segwit v0, so we cannot drop any witness_utxos
                to_drop.clear();
                break;
            }
            if (wit_ver == 0) {
                // Segwit v0, so we cannot drop any non_witness_utxos
                to_drop.clear();
                break;
            }
            if (input.non_witness_utxo) {
                to_drop.push_back(i);
            }
        }

        // Drop the non_witness_utxos that we can drop
        for (unsigned int i : to_drop) {
            pstx.inputs.at(i).non_witness_utxo = nullptr;
        }
    }
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
    return DecodeRawPST(pst, MakeByteSpan(ParseHex(hex_pst)), error);
}

bool DecodeRawPST(PartiallySignedTransaction& pst, Span<const std::byte> tx_data, std::string& error)
{
    CDataStream ss_data(tx_data, SER_NETWORK, PROTOCOL_VERSION);
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
