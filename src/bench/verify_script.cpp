// Copyright (c) 2016-2022 The Bitcoin Core developers
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

#if defined(HAVE_CONFIG_H)
#include <config/freicoin-config.h>
#endif

#include <addresstype.h>
#include <bench/bench.h>
#include <key.h>
#if defined(HAVE_CONSENSUS_LIB)
#include <script/freicoinconsensus.h>
#endif
#include <script/script.h>
#include <script/solver.h>
#include <script/interpreter.h>
#include <streams.h>
#include <test/util/transaction_utils.h>

#include <array>

// Microbenchmark for verification of a basic P2WPK script. Can be easily
// modified to measure performance of other types of scripts.
static void VerifyScriptBench(benchmark::Bench& bench)
{
    ECC_Start();

    const uint32_t flags{SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH};

    // Key pair.
    CKey key;
    static const std::array<unsigned char, 32> vchKey = {
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
        }
    };
    key.Set(vchKey.begin(), vchKey.end(), false);
    CPubKey pubkey = key.GetPubKey();

    // Script.
    CScript p2pk = GetScriptForRawPubKey(pubkey);
    CScript scriptPubKey = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, p2pk));
    CScript scriptSig;
    const CMutableTransaction& txCredit = BuildCreditingTransaction(scriptPubKey, 1);
    CMutableTransaction txSpend = BuildSpendingTransaction(scriptSig, CScriptWitness(), CTransaction(txCredit));
    CScriptWitness& witness = txSpend.vin[0].scriptWitness;
    witness.stack.emplace_back();
    key.Sign(SignatureHash(p2pk, txSpend, 0, SIGHASH_ALL, txCredit.vout[0].GetReferenceValue(), txCredit.lock_height, SigVersion::WITNESS_V0), witness.stack.back());
    witness.stack.back().push_back(static_cast<unsigned char>(SIGHASH_ALL));
    WitnessV0ScriptEntry entry(0 /* version */, p2pk);
    witness.stack.push_back(entry.m_script);
    witness.stack.emplace_back();

    // Benchmark.
    bench.run([&] {
        ScriptError err;
        bool success = VerifyScript(
            txSpend.vin[0].scriptSig,
            txCredit.vout[0].scriptPubKey,
            &txSpend.vin[0].scriptWitness,
            flags,
            MutableTransactionSignatureChecker(&txSpend, 0, txCredit.vout[0].GetReferenceValue(), txCredit.lock_height, MissingDataBehavior::ASSERT_FAIL),
            &err);
        assert(err == SCRIPT_ERR_OK);
        assert(success);

#if defined(HAVE_CONSENSUS_LIB)
        DataStream stream;
        stream << TX_WITH_WITNESS(txSpend);
        int csuccess = freicoinconsensus_verify_script_with_amount(
            txCredit.vout[0].scriptPubKey.data(),
            txCredit.vout[0].scriptPubKey.size(),
            txCredit.vout[0].GetReferenceValue(),
            txCredit.lock_height,
            (const unsigned char*)stream.data(), stream.size(), 0, flags, nullptr);
        assert(csuccess == 1);
#endif
    });
    ECC_Stop();
}

static void VerifyNestedIfScript(benchmark::Bench& bench)
{
    std::vector<std::vector<unsigned char>> stack;
    CScript script;
    for (int i = 0; i < 100; ++i) {
        script << OP_1 << OP_IF;
    }
    for (int i = 0; i < 1000; ++i) {
        script << OP_1;
    }
    for (int i = 0; i < 100; ++i) {
        script << OP_ENDIF;
    }
    bench.run([&] {
        auto stack_copy = stack;
        ScriptError error;
        bool ret = EvalScript(stack_copy, script, 0, BaseSignatureChecker(), SigVersion::BASE, &error);
        assert(ret);
    });
}

BENCHMARK(VerifyScriptBench, benchmark::PriorityLevel::HIGH);
BENCHMARK(VerifyNestedIfScript, benchmark::PriorityLevel::HIGH);
