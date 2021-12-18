// Copyright (c) 2019 The Bitcoin Core developers
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

#include <test/fuzz/fuzz.h>

#include <node/pst.h>
#include <optional.h>
#include <pst.h>
#include <pubkey.h>
#include <script/script.h>
#include <streams.h>
#include <util/memory.h>
#include <version.h>

#include <cstdint>
#include <string>
#include <vector>

void initialize()
{
    static const ECCVerifyHandle verify_handle;
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    PartiallySignedTransaction pst_mut;
    const std::string raw_pst{buffer.begin(), buffer.end()};
    std::string error;
    if (!DecodeRawPST(pst_mut, raw_pst, error)) {
        return;
    }
    const PartiallySignedTransaction pst = pst_mut;

    const PSTAnalysis analysis = AnalyzePST(pst);
    (void)PSTRoleName(analysis.next);
    for (const PSTInputAnalysis& input_analysis : analysis.inputs) {
        (void)PSTRoleName(input_analysis.next);
    }

    (void)pst.IsNull();

    Optional<CMutableTransaction> tx = pst.tx;
    if (tx) {
        const CMutableTransaction& mtx = *tx;
        const PartiallySignedTransaction pst_from_tx{mtx};
    }

    for (const PSTInput& input : pst.inputs) {
        (void)PSTInputSigned(input);
        (void)input.IsNull();
    }

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

    pst_mut = pst;
    (void)pst_mut.Merge(pst);
}
