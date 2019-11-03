// Copyright (c) 2011-2016 The Freicoin developers
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include "data/tx_invalid.json.h"
#include "data/tx_valid.json.h"
#include "test/test_freicoin.h"

#include "clientversion.h"
#include "checkqueue.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "key.h"
#include "keystore.h"
#include "main.h" // For CheckTransaction
#include "policy/policy.h"
#include "script/script.h"
#include "script/sign.h"
#include "script/script_error.h"
#include "script/standard.h"
#include "utilstrencodings.h"

#include <map>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include <univalue.h>

using namespace std;

typedef vector<unsigned char> valtype;

// In script_tests.cpp
extern UniValue read_json(const std::string& jsondata);

static std::map<string, unsigned int> mapFlagNames = boost::assign::map_list_of
    (string("NONE"), (unsigned int)SCRIPT_VERIFY_NONE)
    (string("P2SH"), (unsigned int)SCRIPT_VERIFY_P2SH)
    (string("STRICTENC"), (unsigned int)SCRIPT_VERIFY_STRICTENC)
    (string("DERSIG"), (unsigned int)SCRIPT_VERIFY_DERSIG)
    (string("LOW_S"), (unsigned int)SCRIPT_VERIFY_LOW_S)
    (string("SIGPUSHONLY"), (unsigned int)SCRIPT_VERIFY_SIGPUSHONLY)
    (string("MINIMALDATA"), (unsigned int)SCRIPT_VERIFY_MINIMALDATA)
    (string("DISCOURAGE_UPGRADABLE_NOPS"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS)
    (string("CLEANSTACK"), (unsigned int)SCRIPT_VERIFY_CLEANSTACK)
    (string("NULLFAIL"), (unsigned int)SCRIPT_VERIFY_NULLFAIL)
    (string("MULTISIG_HINT"), (unsigned int)SCRIPT_VERIFY_MULTISIG_HINT)
    (string("WITNESS"), (unsigned int)SCRIPT_VERIFY_WITNESS)
    (string("DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM)
    (string("WITNESS_PUBKEYTYPE"), (unsigned int)SCRIPT_VERIFY_WITNESS_PUBKEYTYPE)
    (string("LOCK_HEIGHT_NOT_UNDER_SIGNATURE"), (unsigned int)SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE);

unsigned int ParseScriptFlags(string strFlags)
{
    if (strFlags.empty()) {
        return 0;
    }
    unsigned int flags = 0;
    vector<string> words;
    boost::algorithm::split(words, strFlags, boost::algorithm::is_any_of(","));

    BOOST_FOREACH(string word, words)
    {
        if (!mapFlagNames.count(word))
            BOOST_ERROR("Bad test: unknown verification flag '" << word << "'");
        flags |= mapFlagNames[word];
    }

    return flags;
}

string FormatScriptFlags(unsigned int flags)
{
    if (flags == 0) {
        return "";
    }
    string ret;
    std::map<string, unsigned int>::const_iterator it = mapFlagNames.begin();
    while (it != mapFlagNames.end()) {
        if (flags & it->second) {
            ret += it->first + ",";
        }
        it++;
    }
    return ret.substr(0, ret.size() - 1);
}

BOOST_FIXTURE_TEST_SUITE(transaction_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(tx_valid)
{
    // Read tests from test/data/tx_valid.json
    // Format is an array of arrays
    // Inner arrays are either [ "comment" ]
    // or [[[prevout hash, prevout index, prevout scriptPubKey], [input 2], ...],"], serializedTransaction, verifyFlags
    // ... where all scripts are stringified scripts.
    //
    // verifyFlags is a comma separated list of script verification flags to apply, or "NONE"
    UniValue tests = read_json(std::string(json_tests::tx_valid, json_tests::tx_valid + sizeof(json_tests::tx_valid)));

    ScriptError err;
    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        string strTest = test.write();
        if (test[0].isArray())
        {
            if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            map<COutPoint, CScript> mapprevOutScriptPubKeys;
            map<COutPoint, int64_t> mapprevOutValues;
            map<COutPoint, int64_t> mapprevOutRefHeights;
            UniValue inputs = test[0].get_array();
            bool fValid = true;
	    for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
	        const UniValue& input = inputs[inpIdx];
                if (!input.isArray())
                {
                    fValid = false;
                    break;
                }
                UniValue vinput = input.get_array();
                if (vinput.size() < 3 || vinput.size() > 4)
                {
                    fValid = false;
                    break;
                }
                COutPoint outpoint(uint256S(vinput[0].get_str()), vinput[1].get_int());
                mapprevOutScriptPubKeys[outpoint] = ParseScript(vinput[2].get_str());
                if (vinput.size() >= 4)
                {
                    mapprevOutValues[outpoint] = vinput[3].get_int64();
                }
                if (vinput.size() >= 5)
                {
                    mapprevOutValues[outpoint] = vinput[4].get_int64();
                }
            }
            if (!fValid)
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            string transaction = test[1].get_str();
            CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION);
            CTransaction tx;
            stream >> tx;
            // Tests imported from bitcoin with witness values use 0x00 for the
            // dummy value, which often results in a successful deserialization
            // with an empty input vector and a single output, and don't use up
            // the serialiation buffer. These tests should fail and be updated.
            BOOST_CHECK(stream.empty());

            CValidationState state;
            BOOST_CHECK_MESSAGE(CheckTransaction(tx, state, false), strTest);
            BOOST_CHECK(state.IsValid());

            PrecomputedTransactionData txdata(tx);
            for (unsigned int i = 0; i < tx.vin.size(); i++)
            {
                if (!mapprevOutScriptPubKeys.count(tx.vin[i].prevout))
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    break;
                }

                CAmount amount = 0;
                if (mapprevOutValues.count(tx.vin[i].prevout)) {
                    amount = mapprevOutValues[tx.vin[i].prevout];
                }
                int64_t refheight = 0;
                if (mapprevOutRefHeights.count(tx.vin[i].prevout)) {
                    refheight = mapprevOutRefHeights[tx.vin[i].prevout];
                }
                unsigned int verify_flags = ParseScriptFlags(test[2].get_str());
                int txsigcheck_flags = TXSIGCHECK_NONE;
                if (verify_flags & SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE) {
                    txsigcheck_flags |= TXSIGCHECK_NO_LOCK_HEIGHT;
                    verify_flags &= ~SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE;
                }
                const CScriptWitness *witness = (i < tx.wit.vtxinwit.size()) ? &tx.wit.vtxinwit[i].scriptWitness : NULL;
                bool valid = VerifyScript(tx.vin[i].scriptSig, mapprevOutScriptPubKeys[tx.vin[i].prevout],
                                          witness, verify_flags, TransactionSignatureChecker(&tx, i, amount, refheight, txdata, txsigcheck_flags), &err);
                BOOST_CHECK_MESSAGE(valid,
                                    strTest + " error: " + ScriptErrorString(err));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK,
                                    strTest + " error: " + ScriptErrorString(err));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(tx_invalid)
{
    // Read tests from test/data/tx_invalid.json
    // Format is an array of arrays
    // Inner arrays are either [ "comment" ]
    // or [[[prevout hash, prevout index, prevout scriptPubKey], [input 2], ...],"], serializedTransaction, verifyFlags
    // ... where all scripts are stringified scripts.
    //
    // verifyFlags is a comma separated list of script verification flags to apply, or "NONE"
    UniValue tests = read_json(std::string(json_tests::tx_invalid, json_tests::tx_invalid + sizeof(json_tests::tx_invalid)));

    ScriptError err;
    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        string strTest = test.write();
        if (test[0].isArray())
        {
            if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            map<COutPoint, CScript> mapprevOutScriptPubKeys;
            map<COutPoint, int64_t> mapprevOutValues;
            map<COutPoint, int64_t> mapprevOutRefHeights;
            UniValue inputs = test[0].get_array();
            bool fValid = true;
	    for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
	        const UniValue& input = inputs[inpIdx];
                if (!input.isArray())
                {
                    fValid = false;
                    break;
                }
                UniValue vinput = input.get_array();
                if (vinput.size() < 3 || vinput.size() > 4)
                {
                    fValid = false;
                    break;
                }
                COutPoint outpoint(uint256S(vinput[0].get_str()), vinput[1].get_int());
                mapprevOutScriptPubKeys[outpoint] = ParseScript(vinput[2].get_str());
                if (vinput.size() >= 4)
                {
                    mapprevOutValues[outpoint] = vinput[3].get_int64();
                }
                if (vinput.size() >= 5)
                {
                    mapprevOutRefHeights[outpoint] = vinput[4].get_int64();
                }
            }
            if (!fValid)
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            string transaction = test[1].get_str();
            CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION );
            CTransaction tx;
            stream >> tx;
            // Tests imported from bitcoin with witness values use 0x00 for the
            // dummy value, which often results in a successful deserialization
            // with an empty input vector and a single output, and don't use up
            // the serialiation buffer. These tests should fail and be updated.
            BOOST_CHECK(stream.empty());

            CValidationState state;
            fValid = CheckTransaction(tx, state, false) && state.IsValid();

            PrecomputedTransactionData txdata(tx);
            for (unsigned int i = 0; i < tx.vin.size() && fValid; i++)
            {
                if (!mapprevOutScriptPubKeys.count(tx.vin[i].prevout))
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    break;
                }

                unsigned int verify_flags = ParseScriptFlags(test[2].get_str());
                int txsigcheck_flags = TXSIGCHECK_NONE;
                if (verify_flags & SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE) {
                    txsigcheck_flags |= TXSIGCHECK_NO_LOCK_HEIGHT;
                    verify_flags &= ~SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE;
                }
                CAmount amount = 0;
                if (mapprevOutValues.count(tx.vin[i].prevout)) {
                    amount = mapprevOutValues[tx.vin[i].prevout];
                }
                int64_t refheight = 0;
                if (mapprevOutRefHeights.count(tx.vin[i].prevout)) {
                    refheight = mapprevOutRefHeights[tx.vin[i].prevout];
                }
                const CScriptWitness *witness = (i < tx.wit.vtxinwit.size()) ? &tx.wit.vtxinwit[i].scriptWitness : NULL;
                fValid = VerifyScript(tx.vin[i].scriptSig, mapprevOutScriptPubKeys[tx.vin[i].prevout],
                                      witness, verify_flags, TransactionSignatureChecker(&tx, i, amount, refheight, txdata, txsigcheck_flags), &err);
            }
            BOOST_CHECK_MESSAGE(!fValid, strTest + " error: " + ScriptErrorString(err));
            BOOST_CHECK_MESSAGE(err != SCRIPT_ERR_OK, strTest + " error: " + ScriptErrorString(err));
        }
    }
}

BOOST_AUTO_TEST_CASE(basic_transaction_tests)
{
    // Random real transaction (b25458e2df302ff1ffaaa83969c22a3b94daba46b87f3c4f31eac153a1d9a31d)
    unsigned char ch[] = {0x02, 0x00, 0x00, 0x00, 0x01, 0x36, 0x0a, 0xc4, 0x73, 0x07, 0x05, 0xd8, 0xaa, 0x9e, 0x64, 0xdb, 0x8e, 0x92, 0x75, 0xe0, 0x1f, 0x9c, 0x63, 0xd1, 0x8c, 0x07, 0xba, 0xdd, 0x36, 0xde, 0x9f, 0x42, 0x67, 0x22, 0xb3, 0x94, 0x40, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x49, 0x30, 0x46, 0x02, 0x21, 0x00, 0xb3, 0xb7, 0x35, 0xd1, 0x69, 0xa8, 0xb9, 0x99, 0x7f, 0x27, 0x83, 0xc8, 0x3d, 0xe2, 0x17, 0x58, 0xe4, 0xf8, 0xd4, 0x41, 0x4d, 0x16, 0xe7, 0x75, 0x1e, 0xde, 0x63, 0x0c, 0x09, 0xaa, 0x01, 0x33, 0x02, 0x21, 0x00, 0xad, 0x5d, 0x77, 0xbb, 0xb2, 0xd8, 0xd8, 0xd0, 0x30, 0x6f, 0x17, 0xec, 0x12, 0x72, 0x48, 0x66, 0x57, 0x19, 0xfb, 0x17, 0x86, 0xab, 0x0c, 0xc3, 0x67, 0xee, 0x69, 0x2b, 0xb6, 0x5e, 0x74, 0x0b, 0x01, 0xff, 0xff, 0xff, 0xff, 0x02, 0x4f, 0x43, 0x62, 0x08, 0x05, 0x00, 0x00, 0x00, 0x19, 0x76, 0xa9, 0x14, 0xe6, 0x7f, 0xa0, 0xa2, 0xcd, 0x71, 0xab, 0x98, 0xa4, 0xfe, 0xb2, 0x36, 0xcf, 0x78, 0xed, 0xfb, 0xf0, 0x62, 0x1a, 0x54, 0x88, 0xac, 0xa6, 0xeb, 0x23, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x19, 0x76, 0xa9, 0x14, 0x90, 0x69, 0x45, 0x10, 0x72, 0x5f, 0x9e, 0x60, 0xe7, 0xc6, 0xf1, 0xb1, 0x5b, 0xe4, 0x09, 0x0e, 0x20, 0xba, 0xb2, 0x57, 0x88, 0xac, 0x00, 0x00, 0x00, 0x00, 0x44, 0x2f, 0x00, 0x00, 0x00};
    vector<unsigned char> vch(ch, ch + sizeof(ch) -1);
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    CMutableTransaction tx;
    stream >> tx;
    CValidationState state;
    BOOST_CHECK_MESSAGE(CheckTransaction(tx, state, false) && state.IsValid(), "Simple deserialized transaction should be valid.");

    // Check that duplicate txins fail
    tx.vin.push_back(tx.vin[0]);
    BOOST_CHECK_MESSAGE(!CheckTransaction(tx, state, false) || !state.IsValid(), "Transaction with duplicate txins should be invalid.");
}

//
// Helper: create two dummy transactions, each with
// two outputs.  The first has 11 and 50 CENT outputs
// paid to a TX_PUBKEY, the second 21 and 22 CENT outputs
// paid to a TX_PUBKEYHASH.
//
static std::vector<CMutableTransaction>
SetupDummyInputs(CBasicKeyStore& keystoreRet, CCoinsViewCache& coinsRet)
{
    std::vector<CMutableTransaction> dummyTransactions;
    dummyTransactions.resize(2);

    // Add some keys to the keystore:
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(i % 2);
        keystoreRet.AddKey(key[i]);
    }

    // Create some dummy input transactions
    dummyTransactions[0].vout.resize(2);
    dummyTransactions[0].vout[0].nValue = 11*CENT;
    dummyTransactions[0].vout[0].scriptPubKey << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    dummyTransactions[0].vout[1].nValue = 50*CENT;
    dummyTransactions[0].vout[1].scriptPubKey << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    coinsRet.ModifyCoins(dummyTransactions[0].GetHash())->FromTx(dummyTransactions[0], 0);

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = 21*CENT;
    dummyTransactions[1].vout[0].scriptPubKey = GetScriptForDestination(key[2].GetPubKey().GetID());
    dummyTransactions[1].vout[1].nValue = 22*CENT;
    dummyTransactions[1].vout[1].scriptPubKey = GetScriptForDestination(key[3].GetPubKey().GetID());
    coinsRet.ModifyCoins(dummyTransactions[1].GetHash())->FromTx(dummyTransactions[1], 0);

    return dummyTransactions;
}

BOOST_AUTO_TEST_CASE(test_Get)
{
    CBasicKeyStore keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    CMutableTransaction t1;
    t1.vin.resize(3);
    t1.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t1.vin[0].prevout.n = 1;
    t1.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t1.vin[1].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[1].prevout.n = 0;
    t1.vin[1].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vin[2].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[2].prevout.n = 1;
    t1.vin[2].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vout.resize(2);
    t1.vout[0].nValue = 90*CENT;
    t1.vout[0].scriptPubKey << OP_1;

    BOOST_CHECK(AreInputsStandard(t1, coins));
    BOOST_CHECK_EQUAL(coins.GetValueIn(t1), (50+21+22)*CENT);
}

void CreateCreditAndSpend(const CKeyStore& keystore, const CScript& outscript, CTransaction& output, CMutableTransaction& input, bool success = true)
{
    CMutableTransaction outputm;
    outputm.nVersion = 1;
    outputm.vin.resize(1);
    outputm.vin[0].prevout.SetNull();
    outputm.vin[0].scriptSig = CScript();
    outputm.wit.vtxinwit.resize(1);
    outputm.vout.resize(1);
    outputm.vout[0].nValue = 1;
    outputm.vout[0].scriptPubKey = outscript;
    CDataStream ssout(SER_NETWORK, PROTOCOL_VERSION);
    ssout << outputm;
    ssout >> output;
    assert(output.vin.size() == 1);
    assert(output.vin[0] == outputm.vin[0]);
    assert(output.vout.size() == 1);
    assert(output.vout[0] == outputm.vout[0]);
    assert(output.wit.vtxinwit.size() == 0);

    CMutableTransaction inputm;
    inputm.nVersion = 1;
    inputm.vin.resize(1);
    inputm.vin[0].prevout.hash = output.GetHash();
    inputm.vin[0].prevout.n = 0;
    inputm.wit.vtxinwit.resize(1);
    inputm.vout.resize(1);
    inputm.vout[0].nValue = 1;
    inputm.vout[0].scriptPubKey = CScript();
    bool ret = SignSignature(keystore, output, inputm, 0, SIGHASH_ALL);
    assert(ret == success);
    CDataStream ssin(SER_NETWORK, PROTOCOL_VERSION);
    ssin << inputm;
    ssin >> input;
    assert(input.vin.size() == 1);
    assert(input.vin[0] == inputm.vin[0]);
    assert(input.vout.size() == 1);
    assert(input.vout[0] == inputm.vout[0]);
    if (inputm.wit.IsNull()) {
        assert(input.wit.IsNull());
    } else {
        assert(!input.wit.IsNull());
        assert(input.wit.vtxinwit.size() == 1);
        assert(input.wit.vtxinwit[0].scriptWitness.stack == inputm.wit.vtxinwit[0].scriptWitness.stack);
    }
}

void CheckWithFlag(const CTransaction& output, const CMutableTransaction& input, int flags, bool success)
{
    ScriptError error;
    CTransaction inputi(input);
    bool ret = VerifyScript(inputi.vin[0].scriptSig, output.vout[0].scriptPubKey, inputi.wit.vtxinwit.size() > 0 ? &inputi.wit.vtxinwit[0].scriptWitness : NULL, flags, TransactionSignatureChecker(&inputi, 0, output.vout[0].nValue, output.lock_height), &error);
    assert(ret == success);
}

static CScript PushAll(const vector<valtype>& values)
{
    CScript result;
    BOOST_FOREACH(const valtype& v, values) {
        if (v.size() == 0) {
            result << OP_0;
        } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
            result << CScript::EncodeOP_N(v[0]);
        } else {
            result << v;
        }
    }
    return result;
}

void ReplaceRedeemScript(CScript& script, const CScript& redeemScript)
{
    vector<valtype> stack;
    EvalScript(stack, script, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SIGVERSION_BASE);
    assert(stack.size() > 0);
    stack.back() = std::vector<unsigned char>(redeemScript.begin(), redeemScript.end());
    script = PushAll(stack);
}

BOOST_AUTO_TEST_CASE(test_big_witness_transaction) {
    CMutableTransaction mtx;
    mtx.nVersion = 1;

    CKey key;
    key.MakeNewKey(true); // Need to use compressed keys in segwit or the signing will fail
    CBasicKeyStore keystore;
    keystore.AddKeyPubKey(key, key.GetPubKey());
    CKeyID hash = key.GetPubKey().GetID();
    CScript scriptPubKey = CScript() << OP_0 << std::vector<unsigned char>(hash.begin(), hash.end());

    vector<int> sigHashes;
    sigHashes.push_back(SIGHASH_NONE | SIGHASH_ANYONECANPAY);
    sigHashes.push_back(SIGHASH_SINGLE | SIGHASH_ANYONECANPAY);
    sigHashes.push_back(SIGHASH_ALL | SIGHASH_ANYONECANPAY);
    sigHashes.push_back(SIGHASH_NONE);
    sigHashes.push_back(SIGHASH_SINGLE);
    sigHashes.push_back(SIGHASH_ALL);

    // create a big transaction of 4500 inputs signed by the same key
    for(uint32_t ij = 0; ij < 4500; ij++) {
        uint32_t i = mtx.vin.size();
        uint256 prevId;
        prevId.SetHex("0000000000000000000000000000000000000000000000000000000000000100");
        COutPoint outpoint(prevId, i);

        mtx.vin.resize(mtx.vin.size() + 1);
        mtx.vin[i].prevout = outpoint;
        mtx.vin[i].scriptSig = CScript();

        mtx.vout.resize(mtx.vout.size() + 1);
        mtx.vout[i].nValue = 1000;
        mtx.vout[i].scriptPubKey = CScript() << OP_1;
    }

    // sign all inputs
    for(uint32_t i = 0; i < mtx.vin.size(); i++) {
        bool hashSigned = SignSignature(keystore, scriptPubKey, mtx, i, 1000, 0, sigHashes.at(i % sigHashes.size()));
        assert(hashSigned);
    }

    CTransaction tx;
    CDataStream ssout(SER_NETWORK, PROTOCOL_VERSION);
    WithOrVersion(&ssout, 0) << mtx;
    WithOrVersion(&ssout, 0) >> tx;

    // check all inputs concurrently, with the cache
    PrecomputedTransactionData txdata(tx);
    boost::thread_group threadGroup;
    CCheckQueue<CScriptCheck> scriptcheckqueue(128);
    CCheckQueueControl<CScriptCheck> control(&scriptcheckqueue);

    for (int i=0; i<20; i++)
        threadGroup.create_thread(boost::bind(&CCheckQueue<CScriptCheck>::Thread, boost::ref(scriptcheckqueue)));

    CCoins coins;
    coins.nVersion = 1;
    coins.fCoinBase = false;
    for(uint32_t i = 0; i < mtx.vin.size(); i++) {
        CTxOut txout;
        txout.nValue = 1000;
        txout.scriptPubKey = scriptPubKey;
        coins.vout.push_back(txout);
    }

    for(uint32_t i = 0; i < mtx.vin.size(); i++) {
        std::vector<CScriptCheck> vChecks;
        CScriptCheck check(coins, tx, i, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false, &txdata);
        vChecks.push_back(CScriptCheck());
        check.swap(vChecks.back());
        control.Add(vChecks);
    }

    bool controlCheck = control.Wait();
    assert(controlCheck);

    threadGroup.interrupt_all();
    threadGroup.join_all();
}

BOOST_AUTO_TEST_CASE(test_witness)
{
    CBasicKeyStore keystore, keystore2;
    CKey key1, key2, key3, key1L, key2L;
    CPubKey pubkey1, pubkey2, pubkey3, pubkey1L, pubkey2L;
    key1.MakeNewKey(true);
    key2.MakeNewKey(true);
    key3.MakeNewKey(true);
    key1L.MakeNewKey(false);
    key2L.MakeNewKey(false);
    pubkey1 = key1.GetPubKey();
    pubkey2 = key2.GetPubKey();
    pubkey3 = key3.GetPubKey();
    pubkey1L = key1L.GetPubKey();
    pubkey2L = key2L.GetPubKey();
    keystore.AddKeyPubKey(key1, pubkey1);
    keystore.AddKeyPubKey(key2, pubkey2);
    keystore.AddKeyPubKey(key1L, pubkey1L);
    keystore.AddKeyPubKey(key2L, pubkey2L);
    CScript scriptPubkey1, scriptPubkey2, scriptPubkey1L, scriptPubkey2L, scriptMulti;
    scriptPubkey1 << ToByteVector(pubkey1) << OP_CHECKSIG;
    scriptPubkey2 << ToByteVector(pubkey2) << OP_CHECKSIG;
    scriptPubkey1L << ToByteVector(pubkey1L) << OP_CHECKSIG;
    scriptPubkey2L << ToByteVector(pubkey2L) << OP_CHECKSIG;
    std::vector<CPubKey> oneandthree;
    oneandthree.push_back(pubkey1);
    oneandthree.push_back(pubkey3);
    scriptMulti = GetScriptForMultisig(2, oneandthree);
    keystore.AddCScript(scriptPubkey1);
    keystore.AddCScript(scriptPubkey2);
    keystore.AddCScript(scriptPubkey1L);
    keystore.AddCScript(scriptPubkey2L);
    keystore.AddCScript(scriptMulti);
    keystore.AddCScript(GetScriptForWitness(scriptPubkey1));
    keystore.AddCScript(GetScriptForWitness(scriptPubkey2));
    keystore.AddCScript(GetScriptForWitness(scriptPubkey1L));
    keystore.AddCScript(GetScriptForWitness(scriptPubkey2L));
    keystore.AddCScript(GetScriptForWitness(scriptMulti));
    keystore2.AddCScript(scriptMulti);
    keystore2.AddCScript(GetScriptForWitness(scriptMulti));
    keystore2.AddKeyPubKey(key3, pubkey3);

    CTransaction output1, output2;
    CMutableTransaction input1, input2;
    SignatureData sigdata;

    // Normal pay-to-compressed-pubkey.
    CreateCreditAndSpend(keystore, scriptPubkey1, output1, input1);
    CreateCreditAndSpend(keystore, scriptPubkey2, output2, input2);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // P2SH pay-to-compressed-pubkey.
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(scriptPubkey1)), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(scriptPubkey2)), output2, input2);
    ReplaceRedeemScript(input2.vin[0].scriptSig, scriptPubkey1);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // Witness pay-to-compressed-pubkey (v0).
    CreateCreditAndSpend(keystore, GetScriptForWitness(scriptPubkey1), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForWitness(scriptPubkey2), output2, input2);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // P2SH witness pay-to-compressed-pubkey (v0).
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(GetScriptForWitness(scriptPubkey1))), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(GetScriptForWitness(scriptPubkey2))), output2, input2);
    ReplaceRedeemScript(input2.vin[0].scriptSig, GetScriptForWitness(scriptPubkey1));
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // Normal pay-to-uncompressed-pubkey.
    CreateCreditAndSpend(keystore, scriptPubkey1L, output1, input1);
    CreateCreditAndSpend(keystore, scriptPubkey2L, output2, input2);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // P2SH pay-to-uncompressed-pubkey.
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(scriptPubkey1L)), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(scriptPubkey2L)), output2, input2);
    ReplaceRedeemScript(input2.vin[0].scriptSig, scriptPubkey1L);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // Signing disabled for witness pay-to-uncompressed-pubkey (v1).
    CreateCreditAndSpend(keystore, GetScriptForWitness(scriptPubkey1L), output1, input1, false);
    CreateCreditAndSpend(keystore, GetScriptForWitness(scriptPubkey2L), output2, input2, false);

    // Signing disabled for P2SH witness pay-to-uncompressed-pubkey (v1).
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(GetScriptForWitness(scriptPubkey1L))), output1, input1, false);
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(GetScriptForWitness(scriptPubkey2L))), output2, input2, false);

    // Normal 2-of-2 multisig
    CreateCreditAndSpend(keystore, scriptMulti, output1, input1, false);
    CheckWithFlag(output1, input1, 0, false);
    CreateCreditAndSpend(keystore2, scriptMulti, output2, input2, false);
    CheckWithFlag(output2, input2, 0, false);
    BOOST_CHECK(output1 == output2);
    UpdateTransaction(input1, 0, CombineSignatures(output1.vout[0].scriptPubKey, MutableTransactionSignatureChecker(&input1, 0, output1.vout[0].nValue, output1.lock_height), DataFromTransaction(input1, 0), DataFromTransaction(input2, 0)));
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);

    // P2SH 2-of-2 multisig
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(scriptMulti)), output1, input1, false);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, false);
    CreateCreditAndSpend(keystore2, GetScriptForDestination(CScriptID(scriptMulti)), output2, input2, false);
    CheckWithFlag(output2, input2, 0, true);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH, false);
    BOOST_CHECK(output1 == output2);
    UpdateTransaction(input1, 0, CombineSignatures(output1.vout[0].scriptPubKey, MutableTransactionSignatureChecker(&input1, 0, output1.vout[0].nValue, output1.lock_height), DataFromTransaction(input1, 0), DataFromTransaction(input2, 0)));
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);

    // Witness 2-of-2 multisig
    CreateCreditAndSpend(keystore, GetScriptForWitness(scriptMulti), output1, input1, false);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    CreateCreditAndSpend(keystore2, GetScriptForWitness(scriptMulti), output2, input2, false);
    CheckWithFlag(output2, input2, 0, true);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    BOOST_CHECK(output1 == output2);

    UpdateTransaction(input1, 0, CombineSignatures(output1.vout[0].scriptPubKey, MutableTransactionSignatureChecker(&input1, 0, output1.vout[0].nValue, output1.lock_height), DataFromTransaction(input1, 0), DataFromTransaction(input2, 0)));
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);

    // P2SH witness 2-of-2 multisig
    CreateCreditAndSpend(keystore, GetScriptForDestination(CScriptID(GetScriptForWitness(scriptMulti))), output1, input1, false);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    CreateCreditAndSpend(keystore2, GetScriptForDestination(CScriptID(GetScriptForWitness(scriptMulti))), output2, input2, false);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    BOOST_CHECK(output1 == output2);
    UpdateTransaction(input1, 0, CombineSignatures(output1.vout[0].scriptPubKey, MutableTransactionSignatureChecker(&input1, 0, output1.vout[0].nValue, output1.lock_height), DataFromTransaction(input1, 0), DataFromTransaction(input2, 0)));
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
}

BOOST_AUTO_TEST_CASE(test_IsStandard)
{
    LOCK(cs_main);
    CBasicKeyStore keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    CMutableTransaction t;
    t.vin.resize(1);
    t.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t.vin[0].prevout.n = 1;
    t.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t.vout.resize(1);
    t.vout[0].nValue = 90*CENT;
    CKey key;
    key.MakeNewKey(true);
    t.vout[0].scriptPubKey = GetScriptForDestination(key.GetPubKey().GetID());

    string reason;
    BOOST_CHECK(IsStandardTx(t, reason));

    // Check dust with default relay fee:
    CAmount nDustThreshold = 182 * minRelayTxFee.GetFeePerK()/1000 * 3;
    BOOST_CHECK_EQUAL(nDustThreshold, 546);
    // dust:
    t.vout[0].nValue = nDustThreshold - 1;
    BOOST_CHECK(!IsStandardTx(t, reason));
    // not dust:
    t.vout[0].nValue = nDustThreshold;
    BOOST_CHECK(IsStandardTx(t, reason));

    // Check dust with odd relay fee to verify rounding:
    // nDustThreshold = 182 * 1234 / 1000 * 3
    minRelayTxFee = CFeeRate(1234);
    // dust:
    t.vout[0].nValue = 672 - 1;
    BOOST_CHECK(!IsStandardTx(t, reason));
    // not dust:
    t.vout[0].nValue = 672;
    BOOST_CHECK(IsStandardTx(t, reason));
    minRelayTxFee = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);

    t.vout[0].scriptPubKey = CScript() << OP_1;
    BOOST_CHECK(!IsStandardTx(t, reason));

    // MAX_OP_RETURN_RELAY-byte TX_NULL_DATA (non-standard since removal of '-datacarrier')
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a671");
    BOOST_CHECK_EQUAL(56, t.vout[0].scriptPubKey.size());
    BOOST_CHECK(!IsStandardTx(t, reason));

    // MAX_OP_RETURN_RELAY+1-byte TX_NULL_DATA (non-standard always)
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130");
    BOOST_CHECK_EQUAL(56 + 1, t.vout[0].scriptPubKey.size());
    BOOST_CHECK(!IsStandardTx(t, reason));

    // Data payload can be encoded in any way...
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("");
    BOOST_CHECK(!IsStandardTx(t, reason));
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("00") << ParseHex("01");
    BOOST_CHECK(!IsStandardTx(t, reason));
    // OP_RESERVED *is* considered to be a PUSHDATA type opcode by IsPushOnly()!
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << OP_RESERVED << -1 << 0 << ParseHex("01") << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16;
    BOOST_CHECK(!IsStandardTx(t, reason));
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << 0 << ParseHex("01") << 2 << ParseHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    BOOST_CHECK(!IsStandardTx(t, reason));

    // ...so long as it only contains PUSHDATA's
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << OP_RETURN;
    BOOST_CHECK(!IsStandardTx(t, reason));

    // TX_UNSPENDABLE: TX_NULL_DATA from bitcoin w/o PUSHDATA
    t.vout.resize(1);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(IsStandardTx(t, reason));

    // Only one TX_UNSPENDABLE permitted in all cases
    t.vout.resize(2);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    BOOST_CHECK(!IsStandardTx(t, reason));

    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(!IsStandardTx(t, reason));

    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(!IsStandardTx(t, reason));
}

BOOST_AUTO_TEST_SUITE_END()
