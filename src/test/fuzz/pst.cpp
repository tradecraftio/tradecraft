// Copyright (c) 2019-2021 The Bitcoin Core developers
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

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <node/pst.h>
#include <pst.h>
#include <pubkey.h>
#include <script/script.h>
#include <streams.h>
#include <util/check.h>
#include <version.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using node::AnalyzePST;
using node::PSTAnalysis;
using node::PSTInputAnalysis;

void initialize_pst()
{
    static const ECCVerifyHandle verify_handle;
}

FUZZ_TARGET_INIT(pst, initialize_pst)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    PartiallySignedTransaction pst_mut;
    std::string error;
    auto str = fuzzed_data_provider.ConsumeRandomLengthString();
    if (!DecodeRawPST(pst_mut, MakeByteSpan(str), error)) {
        return;
    }
    const PartiallySignedTransaction pst = pst_mut;

    const PSTAnalysis analysis = AnalyzePST(pst);
    (void)PSTRoleName(analysis.next);
    for (const PSTInputAnalysis& input_analysis : analysis.inputs) {
        (void)PSTRoleName(input_analysis.next);
    }

    (void)pst.IsNull();

    std::optional<CMutableTransaction> tx = pst.tx;
    if (tx) {
        const CMutableTransaction& mtx = *tx;
        const PartiallySignedTransaction pst_from_tx{mtx};
    }

    for (const PSTInput& input : pst.inputs) {
        (void)PSTInputSigned(input);
        (void)input.IsNull();
    }
    (void)CountPSTUnsignedInputs(pst);

    for (const PSTOutput& output : pst.outputs) {
        (void)output.IsNull();
    }

    for (size_t i = 0; i < pst.tx->vin.size(); ++i) {
        CTxOut tx_out;
        if (pst.GetInputUTXO(tx_out, i)) {
            (void)tx_out.IsNull();
            (void)tx_out.ToString();
        }
    }

    pst_mut = pst;
    (void)FinalizePST(pst_mut);

    pst_mut = pst;
    CMutableTransaction result;
    if (FinalizeAndExtractPST(pst_mut, result)) {
        const PartiallySignedTransaction pst_from_tx{result};
    }

    PartiallySignedTransaction pst_merge;
    str = fuzzed_data_provider.ConsumeRandomLengthString();
    if (!DecodeRawPST(pst_merge, MakeByteSpan(str), error)) {
        pst_merge = pst;
    }
    pst_mut = pst;
    (void)pst_mut.Merge(pst_merge);
    pst_mut = pst;
    (void)CombinePSTs(pst_mut, {pst_mut, pst_merge});
    pst_mut = pst;
    for (unsigned int i = 0; i < pst_merge.tx->vin.size(); ++i) {
        (void)pst_mut.AddInput(pst_merge.tx->vin[i], pst_merge.inputs[i]);
    }
    for (unsigned int i = 0; i < pst_merge.tx->vout.size(); ++i) {
        Assert(pst_mut.AddOutput(pst_merge.tx->vout[i], pst_merge.outputs[i]));
    }
    pst_mut.unknown.insert(pst_merge.unknown.begin(), pst_merge.unknown.end());
}
