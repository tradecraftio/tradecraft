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

#ifndef FREICOIN_NODE_PST_H
#define FREICOIN_NODE_PST_H

#include <pst.h>

#include <optional>

namespace node {
/**
 * Holds an analysis of one input from a PST
 */
struct PSTInputAnalysis {
    bool has_utxo; //!< Whether we have UTXO information for this input
    bool is_final; //!< Whether the input has all required information including signatures
    PSTRole next; //!< Which of the BIP 174 roles needs to handle this input next

    std::vector<CKeyID> missing_pubkeys; //!< Pubkeys whose BIP32 derivation path is missing
    std::vector<CKeyID> missing_sigs;    //!< Pubkeys whose signatures are missing
    uint160 missing_redeem_script;       //!< Hash160 of redeem script, if missing
    WitnessV0ScriptHash missing_witness_script;      //!< SHA256 of witness script, if missing
};

/**
 * Holds the results of AnalyzePST (miscellaneous information about a PST)
 */
struct PSTAnalysis {
    std::optional<size_t> estimated_vsize;      //!< Estimated weight of the transaction
    std::optional<CFeeRate> estimated_feerate;  //!< Estimated feerate (fee / weight) of the transaction
    std::optional<CAmount> demurrage;           //!< Amount of inputs lost to demurrage
    std::optional<CAmount> fee;                 //!< Amount of fee being paid by the transaction
    std::vector<PSTInputAnalysis> inputs;      //!< More information about the individual inputs of the transaction
    PSTRole next;                              //!< Which of the BIP 174 roles needs to handle the transaction next
    std::string error;                          //!< Error message

    void SetInvalid(std::string err_msg)
    {
        estimated_vsize = std::nullopt;
        estimated_feerate = std::nullopt;
        fee = std::nullopt;
        inputs.clear();
        next = PSTRole::CREATOR;
        error = err_msg;
    }
};

/**
 * Provides helpful miscellaneous information about where a PST is in the signing workflow.
 *
 * @param[in] pstx the PST to analyze
 * @return A PSTAnalysis with information about the provided PST.
 */
PSTAnalysis AnalyzePST(PartiallySignedTransaction pstx);
} // namespace node

#endif // FREICOIN_NODE_PST_H
