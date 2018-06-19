// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/script.h>

#include <cassert>
#include <ios>
#include <utility>
#include <vector>

FUZZ_TARGET(script_flags)
{
    if (buffer.size() > 100'000) return;
    DataStream ds{buffer};
    try {
        const CTransaction tx(deserialize, TX_WITH_WITNESS, ds);

        unsigned int verify_flags;
        ds >> verify_flags;

        if (!IsValidFlagCombination(verify_flags)) return;

        unsigned int fuzzed_flags;
        ds >> fuzzed_flags;

        std::vector<SpentOutput> spent_outputs;
        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            CTxOut prevout;
            ds >> prevout;
            if (!MoneyRange(prevout.nValue)) {
                // prevouts should be consensus-valid
                prevout.nValue = 1;
            }
            spent_outputs.emplace_back(prevout, tx.lock_height);
        }
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::move(spent_outputs));

        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            const SpentOutput& prevout = txdata.m_spent_outputs.at(i);
            const TransactionSignatureChecker checker{&tx, i, prevout.out.nValue, prevout.refheight, txdata, MissingDataBehavior::ASSERT_FAIL};

            ScriptError serror;
            const bool ret = VerifyScript(tx.vin.at(i).scriptSig, prevout.out.scriptPubKey, &tx.vin.at(i).scriptWitness, verify_flags, checker, &serror);
            assert(ret == (serror == SCRIPT_ERR_OK));

            // Verify that removing flags from a passing test or adding flags to a failing test does not change the result
            if (ret) {
                verify_flags &= ~fuzzed_flags;
            } else {
                verify_flags |= fuzzed_flags;
            }
            if (!IsValidFlagCombination(verify_flags)) return;

            ScriptError serror_fuzzed;
            const bool ret_fuzzed = VerifyScript(tx.vin.at(i).scriptSig, prevout.out.scriptPubKey, &tx.vin.at(i).scriptWitness, verify_flags, checker, &serror_fuzzed);
            assert(ret_fuzzed == (serror_fuzzed == SCRIPT_ERR_OK));

            assert(ret_fuzzed == ret);
        }
    } catch (const std::ios_base::failure&) {
        return;
    }
}
