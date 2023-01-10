// Copyright (c) 2020 The Bitcoin Core developers
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

#include <chainparams.h>
#include <external_signer.h>
#include <wallet/external_signer_scriptpubkeyman.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

bool ExternalSignerScriptPubKeyMan::SetupDescriptor(std::unique_ptr<Descriptor> desc)
{
    LOCK(cs_desc_man);
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER));

    int64_t creation_time = GetTime();

    // Make the descriptor
    WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
    m_wallet_descriptor = w_desc;

    // Store the descriptor
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }

    // TopUp
    TopUp();

    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

ExternalSigner ExternalSignerScriptPubKeyMan::GetExternalSigner() {
    const std::string command = gArgs.GetArg("-signer", "");
    if (command == "") throw std::runtime_error(std::string(__func__) + ": restart freicoind with -signer=<cmd>");
    std::vector<ExternalSigner> signers;
    ExternalSigner::Enumerate(command, signers, Params().NetworkIDString());
    if (signers.empty()) throw std::runtime_error(std::string(__func__) + ": No external signers found");
    // TODO: add fingerprint argument in case of multiple signers
    return signers[0];
}

bool ExternalSignerScriptPubKeyMan::DisplayAddress(const CScript scriptPubKey, const ExternalSigner &signer) const
{
    // TODO: avoid the need to infer a descriptor from inside a descriptor wallet
    auto provider = GetSolvingProvider(scriptPubKey);
    auto descriptor = InferDescriptor(scriptPubKey, *provider);

    signer.DisplayAddress(descriptor->ToString());
    // TODO inspect result
    return true;
}

// If sign is true, transaction must previously have been filled
TransactionError ExternalSignerScriptPubKeyMan::FillPST(PartiallySignedTransaction& pst, const PrecomputedTransactionData& txdata, int sighash_type, bool sign, bool bip32derivs, int* n_signed) const
{
    if (!sign) {
        return DescriptorScriptPubKeyMan::FillPST(pst, txdata, sighash_type, false, bip32derivs, n_signed);
    }

    // Already complete if every input is now signed
    bool complete = true;
    for (const auto& input : pst.inputs) {
        // TODO: for multisig wallets, we should only care if all _our_ inputs are signed
        complete &= PSTInputSigned(input);
    }
    if (complete) return TransactionError::OK;

    std::string strFailReason;
    if(!GetExternalSigner().SignTransaction(pst, strFailReason)) {
        tfm::format(std::cerr, "Failed to sign: %s\n", strFailReason);
        return TransactionError::EXTERNAL_SIGNER_FAILED;
    }
    FinalizePST(pst); // This won't work in a multisig setup
    return TransactionError::OK;
}
