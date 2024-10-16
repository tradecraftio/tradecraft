// Copyright (c) 2011-2022 The Bitcoin Core developers
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

#include <test/util/setup_common.h>

#include <key.h>
#include <policy/policy.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <uint256.h>


#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(multisig_tests, BasicTestingSetup)

static CScript
sign_multisig(const CScript& scriptPubKey, const std::vector<CKey>& keys, int64_t hint, const CTransaction& transaction, int whichIn)
{
    uint256 hash = SignatureHash(scriptPubKey, transaction, whichIn, SIGHASH_ALL, 0, 0, SigVersion::BASE);

    CScript result;
    // Real signing code should use the MultiSigHint class to generate this
    // value. We push a serialized integer representation only as part of the
    // test plan for that code.
    result << hint;
    for (const CKey &key : keys)
    {
        std::vector<unsigned char> vchSig;
        BOOST_CHECK(key.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        result << vchSig;
    }
    return result;
}

BOOST_AUTO_TEST_CASE(multisig_verify)
{
    unsigned int flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC;
    unsigned int validsigs_flags = flags | SCRIPT_VERIFY_NULLFAIL | SCRIPT_VERIFY_MULTISIG_HINT;

    ScriptError err;
    CKey key[4];
    CAmount amount = 0;
    for (int i = 0; i < 4; i++)
        key[i].MakeNewKey(true);

    CScript a_and_b;
    a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

    CScript a_or_b;
    a_or_b << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

    CScript escrow;
    escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;

    CMutableTransaction txFrom;  // Funding transaction
    txFrom.vout.resize(3);
    txFrom.vout[0].scriptPubKey = a_and_b;
    txFrom.vout[1].scriptPubKey = a_or_b;
    txFrom.vout[2].scriptPubKey = escrow;

    CMutableTransaction txTo[3]; // Spending transaction
    for (int i = 0; i < 3; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1;
    }

    std::vector<CKey> keys;
    CScript s;

    // Test a AND b:
    keys.assign(1,key[0]);
    keys.push_back(key[1]);
    s = sign_multisig(a_and_b, keys, 0, CTransaction(txTo[0]), 0);
    BOOST_CHECK(VerifyScript(s, a_and_b, nullptr, flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
    BOOST_CHECK(VerifyScript(s, a_and_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    for (int i = 0; i < 4; i++)
    {
        keys.assign(1,key[i]);
        s = sign_multisig(a_and_b, keys, 0, CTransaction(txTo[0]), 0);
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, nullptr, flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a&b 1: %d", i));
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_INVALID_STACK_OPERATION, ScriptErrorString(err));
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a&b 1: %d", i));
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_INVALID_STACK_OPERATION, ScriptErrorString(err));
        s = sign_multisig(a_and_b, keys, 1, CTransaction(txTo[0]), 0);
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, NULL, flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a&b 3: %d", i));
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_INVALID_STACK_OPERATION, ScriptErrorString(err));
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a&b 3: %d", i));
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_INVALID_STACK_OPERATION, ScriptErrorString(err));

        keys.assign(1,key[1]);
        keys.push_back(key[i]);
        s = sign_multisig(a_and_b, keys, 0, CTransaction(txTo[0]), 0);
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, nullptr, flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a&b 2: %d", i));
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
        BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a&b 2: %d", i));
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_FAILED_SIGNATURE_CHECK, ScriptErrorString(err));
        for (int j = 1; j < 5; ++j) {
            s = sign_multisig(a_and_b, keys, j, CTransaction(txTo[0]), 0);
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, NULL, flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a&b 4: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_and_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[0], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a&b 4: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_MULTISIG_HINT, ScriptErrorString(err));
        }
    }

    // Test a OR b:
    for (int i = 0; i < 4; i++)
    {
        keys.assign(1,key[i]);
        s = sign_multisig(a_or_b, keys, 0, CTransaction(txTo[1]), 0);
        if (i == 0 || i == 1)
        {
            BOOST_CHECK_MESSAGE(VerifyScript(s, a_or_b, nullptr, flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a|b: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_or_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a|b 1: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_MULTISIG_HINT, ScriptErrorString(err));
        }
        else
        {
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_or_b, nullptr, flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a|b: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_or_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a|b 2: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_MULTISIG_HINT, ScriptErrorString(err));
        }
        int hint = 1 + (i%2);
        s = sign_multisig(a_or_b, keys, hint, CTransaction(txTo[1]), 0);
        if (i == 0 || i == 1)
        {
            BOOST_CHECK_MESSAGE(VerifyScript(s, a_or_b, NULL, flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a|b 3: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
            BOOST_CHECK_MESSAGE(VerifyScript(s, a_or_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a|b 3: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
        }
        else
        {
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_or_b, NULL, flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a|b 4: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
            BOOST_CHECK_MESSAGE(!VerifyScript(s, a_or_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("a|b 4: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_FAILED_SIGNATURE_CHECK, ScriptErrorString(err));
        }
    }
    s.clear();
    s << OP_0 << OP_1;
    BOOST_CHECK(!VerifyScript(s, a_or_b, nullptr, flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_SIG_DER, ScriptErrorString(err));
    BOOST_CHECK(!VerifyScript(s, a_or_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_MULTISIG_HINT, ScriptErrorString(err));
    s.clear();
    s << OP_1 << OP_1;
    BOOST_CHECK(!VerifyScript(s, a_or_b, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[1], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_SIG_DER, ScriptErrorString(err));


    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            keys.assign(1,key[i]);
            keys.push_back(key[j]);
            s = sign_multisig(escrow, keys, 0, CTransaction(txTo[2]), 0);
            if (i < j && i < 3 && j < 3)
            {
                BOOST_CHECK_MESSAGE(VerifyScript(s, escrow, nullptr, flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("escrow 1: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
                BOOST_CHECK_MESSAGE(!VerifyScript(s, escrow, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("escrow 1: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_MULTISIG_HINT, ScriptErrorString(err));
            }
            else
            {
                BOOST_CHECK_MESSAGE(!VerifyScript(s, escrow, nullptr, flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("escrow 2: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
                BOOST_CHECK_MESSAGE(!VerifyScript(s, escrow, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("escrow 2: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_MULTISIG_HINT, ScriptErrorString(err));
            }
            int hint = 7 & ~((1 << (2-(i%3))) | (1 << (2-(j%3))));
            s = sign_multisig(escrow, keys, hint, CTransaction(txTo[2]), 0);
            if (i < j && i < 3 && j < 3)
            {
                BOOST_CHECK_MESSAGE(VerifyScript(s, escrow, NULL, flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("escrow 3: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
                BOOST_CHECK_MESSAGE(VerifyScript(s, escrow, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("escrow 3: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
            }
            else
            {
                BOOST_CHECK_MESSAGE(!VerifyScript(s, escrow, NULL, flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("escrow 4: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
                BOOST_CHECK_MESSAGE(!VerifyScript(s, escrow, NULL, validsigs_flags, MutableTransactionSignatureChecker(&txTo[2], 0, amount, txFrom.lock_height, MissingDataBehavior::ASSERT_FAIL), &err), strprintf("escrow 4: %d %d", i, j));
                if ((i%3) == (j%3)) {
                    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_MULTISIG_HINT, ScriptErrorString(err));
                } else {
                    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_FAILED_SIGNATURE_CHECK, ScriptErrorString(err));
                }
            }
        }
}

BOOST_AUTO_TEST_CASE(multisig_IsStandard)
{
    CKey key[4];
    for (int i = 0; i < 4; i++)
        key[i].MakeNewKey(true);

    const auto is_standard{[](const CScript& spk) {
        TxoutType type;
        bool res{::IsStandard(spk, std::nullopt, type)};
        if (res) {
            BOOST_CHECK_EQUAL(type, TxoutType::MULTISIG);
        }
        return res;
    }};

    CScript a_and_b;
    a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK(is_standard(a_and_b));

    CScript a_or_b;
    a_or_b  << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK(is_standard(a_or_b));

    CScript escrow;
    escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;
    BOOST_CHECK(is_standard(escrow));

    CScript one_of_four;
    one_of_four << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << ToByteVector(key[3].GetPubKey()) << OP_4 << OP_CHECKMULTISIG;
    BOOST_CHECK(!is_standard(one_of_four));

    CScript malformed[6];
    malformed[0] << OP_3 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
    malformed[1] << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;
    malformed[2] << OP_0 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
    malformed[3] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_0 << OP_CHECKMULTISIG;
    malformed[4] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_CHECKMULTISIG;
    malformed[5] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_NOP;

    for (int i = 0; i < 6; i++) {
        BOOST_CHECK(!is_standard(malformed[i]));
    }
}

BOOST_AUTO_TEST_CASE(multisig_Sign)
{
    // Test SignSignature() (and therefore the version of Solver() that signs transactions)
    FillableSigningProvider keystore;
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(true);
        BOOST_CHECK(keystore.AddKey(key[i]));
    }

    CScript a_and_b;
    a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

    CScript a_or_b;
    a_or_b  << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

    CScript escrow;
    escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;

    CMutableTransaction txFrom;  // Funding transaction
    txFrom.vout.resize(3);
    txFrom.vout[0].scriptPubKey = a_and_b;
    txFrom.vout[1].scriptPubKey = a_or_b;
    txFrom.vout[2].scriptPubKey = escrow;

    CMutableTransaction txTo[3]; // Spending transaction
    for (int i = 0; i < 3; i++)
    {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout.n = i;
        txTo[i].vin[0].prevout.hash = txFrom.GetHash();
        txTo[i].vout[0].nValue = 1;
    }

    for (int i = 0; i < 3; i++)
    {
        SignatureData empty;
        BOOST_CHECK_MESSAGE(SignSignature(keystore, CTransaction(txFrom), txTo[i], 0, SIGHASH_ALL, empty), strprintf("SignSignature %d", i));
    }
}


BOOST_AUTO_TEST_SUITE_END()
