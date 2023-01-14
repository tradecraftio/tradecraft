// Copyright (c) 2011-2020 The Bitcoin Core developers
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

#include <test/data/tx_invalid.json.h>
#include <test/data/tx_valid.json.h>
#include <test/util/setup_common.h>

#include <checkqueue.h>
#include <clientversion.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <streams.h>
#include <test/util/script.h>
#include <test/util/transaction_utils.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <validation.h>

#include <functional>
#include <map>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/test/unit_test.hpp>

#include <univalue.h>

typedef std::vector<unsigned char> valtype;

// In script_tests.cpp
UniValue read_json(const std::string& jsondata);
std::string FormatScriptError(ScriptError_t err);

static std::map<std::string, unsigned int> mapFlagNames = {
    {std::string("P2SH"), (unsigned int)SCRIPT_VERIFY_P2SH},
    {std::string("STRICTENC"), (unsigned int)SCRIPT_VERIFY_STRICTENC},
    {std::string("DERSIG"), (unsigned int)SCRIPT_VERIFY_DERSIG},
    {std::string("LOW_S"), (unsigned int)SCRIPT_VERIFY_LOW_S},
    {std::string("SIGPUSHONLY"), (unsigned int)SCRIPT_VERIFY_SIGPUSHONLY},
    {std::string("MINIMALDATA"), (unsigned int)SCRIPT_VERIFY_MINIMALDATA},
    {std::string("DISCOURAGE_UPGRADABLE_NOPS"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS},
    {std::string("DISCOURAGE_OP_SUCCESS"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS},
    {std::string("CLEANSTACK"), (unsigned int)SCRIPT_VERIFY_CLEANSTACK},
    {std::string("MINIMALIF"), (unsigned int)SCRIPT_VERIFY_MINIMALIF},
    {std::string("NULLFAIL"), (unsigned int)SCRIPT_VERIFY_NULLFAIL},
    {std::string("MULTISIG_HINT"), (unsigned int)SCRIPT_VERIFY_MULTISIG_HINT},
    {std::string("WITNESS"), (unsigned int)SCRIPT_VERIFY_WITNESS},
    {std::string("DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM},
    {std::string("WITNESS_PUBKEYTYPE"), (unsigned int)SCRIPT_VERIFY_WITNESS_PUBKEYTYPE},
    {std::string("CONST_SCRIPTCODE"), (unsigned int)SCRIPT_VERIFY_CONST_SCRIPTCODE},
    {std::string("TAPROOT"), (unsigned int)SCRIPT_VERIFY_TAPROOT},
    {std::string("DISCOURAGE_UPGRADABLE_PUBKEYTYPE"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE},
    {std::string("DISCOURAGE_OP_SUCCESS"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS},
    {std::string("DISCOURAGE_UPGRADABLE_TAPROOT_VERSION"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION},
    {std::string("LOCK_HEIGHT_NOT_UNDER_SIGNATURE"), (unsigned int)SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE},
};

unsigned int ParseScriptFlags(std::string strFlags)
{
    if (strFlags.empty() || strFlags == "NONE") return 0;
    unsigned int flags = 0;
    std::vector<std::string> words;
    boost::algorithm::split(words, strFlags, boost::algorithm::is_any_of(","));

    for (const std::string& word : words)
    {
        if (!mapFlagNames.count(word))
            BOOST_ERROR("Bad test: unknown verification flag '" << word << "'");
        flags |= mapFlagNames[word];
    }

    return flags;
}

// Contained in script_tests.cpp
extern std::string FormatScriptError(ScriptError err);
extern ScriptError ParseScriptError(const std::string& name);

// Check that all flags in STANDARD_SCRIPT_VERIFY_FLAGS are present in mapFlagNames.
bool CheckMapFlagNames()
{
    unsigned int standard_flags_missing{STANDARD_SCRIPT_VERIFY_FLAGS};
    for (const auto& pair : mapFlagNames) {
        standard_flags_missing &= ~(pair.second);
    }
    return standard_flags_missing == 0;
}

std::string FormatScriptFlags(unsigned int flags)
{
    if (flags == 0) {
        return "";
    }
    std::string ret;
    std::map<std::string, unsigned int>::const_iterator it = mapFlagNames.begin();
    while (it != mapFlagNames.end()) {
        if (flags & it->second) {
            ret += it->first + ",";
        }
        it++;
    }
    return ret.substr(0, ret.size() - 1);
}

/*
* Check that the input scripts of a transaction are valid/invalid as expected.
*/
bool CheckTxScripts(const CTransaction& tx, const std::map<COutPoint, CScript>& map_prevout_scriptPubKeys,
    const std::map<COutPoint, std::pair<CAmount, uint32_t>>& map_prevout_values, unsigned int flags,
    const PrecomputedTransactionData& txdata, TxSigCheckOpt txsigcheck_opts,
    const std::string& strTest, bool expect_valid, std::optional<ScriptError> expected_err = std::nullopt)
{
    bool tx_valid = true;
    ScriptError err = expect_valid ? SCRIPT_ERR_UNKNOWN_ERROR : SCRIPT_ERR_OK;
    for (unsigned int i = 0; i < tx.vin.size() && tx_valid; ++i) {
        const CTxIn input = tx.vin[i];
        CAmount amount = 0;
        uint32_t refheight = 0;
        if (map_prevout_values.count(input.prevout)) {
            const auto& prevout_value = map_prevout_values.at(input.prevout);
            amount = prevout_value.first;
            refheight = prevout_value.second;
        }
        try {
            tx_valid = VerifyScript(input.scriptSig, map_prevout_scriptPubKeys.at(input.prevout),
                &input.scriptWitness, flags, TransactionSignatureChecker(&tx, i, amount, refheight, txdata, MissingDataBehavior::ASSERT_FAIL, txsigcheck_opts), &err);
        } catch (...) {
            BOOST_ERROR("Bad test: " << strTest);
            return true; // The test format is bad and an error is thrown. Return true to silence further error.
        }
        if (expect_valid) {
            BOOST_CHECK_MESSAGE(tx_valid, strTest);
            BOOST_CHECK_MESSAGE((err == SCRIPT_ERR_OK), ScriptErrorString(err));
            err = SCRIPT_ERR_UNKNOWN_ERROR;
        }
    }
    if (!expect_valid) {
        BOOST_CHECK_MESSAGE(!tx_valid, strTest);
        if (expected_err) {
            BOOST_CHECK_MESSAGE(err == expected_err.value(), FormatScriptError(err) + " where " + FormatScriptError(expected_err.value()) + " expected: " + strTest);
        } else {
            BOOST_CHECK_MESSAGE((err != SCRIPT_ERR_OK), ScriptErrorString(err));
        }
    }
    return (tx_valid == expect_valid);
}

/*
 * Trim or fill flags to make the combination valid:
 * WITNESS must be used with P2SH
 * CLEANSTACK must be used WITNESS and P2SH
 */

unsigned int TrimFlags(unsigned int flags)
{
    // Clear verification flags related to protocl rule changes
    flags &= ~(unsigned int)SCRIPT_VERIFY_PROTOCOL_CLEANUP;
    flags &= ~(unsigned int)SCRIPT_VERIFY_SIZE_EXPANSION;

    // WITNESS requires P2SH
    if (!(flags & SCRIPT_VERIFY_P2SH)) flags &= ~(unsigned int)SCRIPT_VERIFY_WITNESS;

    // CLEANSTACK requires WITNESS (and transitively CLEANSTACK requires P2SH)
    if (!(flags & SCRIPT_VERIFY_WITNESS)) flags &= ~(unsigned int)SCRIPT_VERIFY_CLEANSTACK;
    Assert(IsValidFlagCombination(flags));
    return flags;
}

unsigned int FillFlags(unsigned int flags)
{
    // CLEANSTACK implies WITNESS
    if (flags & SCRIPT_VERIFY_CLEANSTACK) flags |= SCRIPT_VERIFY_WITNESS;

    // WITNESS implies P2SH (and transitively CLEANSTACK implies P2SH)
    if (flags & SCRIPT_VERIFY_WITNESS) flags |= SCRIPT_VERIFY_P2SH;
    Assert(IsValidFlagCombination(flags));
    return flags;
}

// Exclude each possible script verify flag from flags. Returns a set of these flag combinations
// that are valid and without duplicates. For example: if flags=1111 and the 4 possible flags are
// 0001, 0010, 0100, and 1000, this should return the set {0111, 1011, 1101, 1110}.
// Assumes that mapFlagNames contains all script verify flags.
std::set<unsigned int> ExcludeIndividualFlags(unsigned int flags)
{
    std::set<unsigned int> flags_combos;
    for (const auto& pair : mapFlagNames) {
        const unsigned int flags_excluding_one = TrimFlags(flags & ~(pair.second));
        if (flags != flags_excluding_one) {
            flags_combos.insert(flags_excluding_one);
        }
    }
    return flags_combos;
}

BOOST_FIXTURE_TEST_SUITE(transaction_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(tx_valid)
{
    BOOST_CHECK_MESSAGE(CheckMapFlagNames(), "mapFlagNames is missing a script verification flag");
    // Read tests from test/data/tx_valid.json
    UniValue tests = read_json(std::string(json_tests::tx_valid, json_tests::tx_valid + sizeof(json_tests::tx_valid)));

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test[0].isArray())
        {
            if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::map<COutPoint, CScript> mapprevOutScriptPubKeys;
            std::map<COutPoint, std::pair<CAmount, uint32_t>> mapprevOutValues;
            UniValue inputs = test[0].get_array();
            bool fValid = true;
            for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
                const UniValue& input = inputs[inpIdx];
                if (!input.isArray()) {
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
                    mapprevOutValues[outpoint] = {vinput[3].get_int64(), 0};
                }
                if (vinput.size() >= 5)
                {
                    int64_t refheight = vinput[4].get_int64();
                    assert(refheight >= std::numeric_limits<uint32_t>::min());
                    assert(refheight <= std::numeric_limits<uint32_t>::max());
                    mapprevOutValues[outpoint].second = static_cast<uint32_t>(refheight);
                }
            }
            if (!fValid)
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::string transaction = test[1].get_str();
            CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION);
            CTransaction tx(deserialize, stream);
            // Tests imported from bitcoin with witness values use 0x00 for the
            // dummy value, which often results in a successful deserialization
            // with an empty input vector and a single output, and don't use up
            // the serialiation buffer. These tests should fail and be updated.
            BOOST_CHECK(stream.empty());

            TxValidationState state;
            BOOST_CHECK_MESSAGE(CheckTransaction(tx, state, Consensus::NONE), strTest);
            BOOST_CHECK(state.IsValid());

            PrecomputedTransactionData txdata(tx);
            unsigned int verify_flags = ParseScriptFlags(test[2].get_str());
            TxSigCheckOpt txsigcheck_opts = TxSigCheckOpt::NONE;
            if (verify_flags & SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE) {
                txsigcheck_opts = TxSigCheckOpt::NO_LOCK_HEIGHT;
                verify_flags &= ~SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE;
            }

            // Check that the test gives a valid combination of flags (otherwise VerifyScript will throw). Don't edit the flags.
            if (~verify_flags != FillFlags(~verify_flags)) {
                BOOST_ERROR("Bad test flags: " << strTest);
            }

            BOOST_CHECK_MESSAGE(CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, TrimFlags(~verify_flags), txdata, txsigcheck_opts, strTest, /* expect_valid */ true),
                                "Tx unexpectedly failed: " << strTest);

            // Backwards compatibility of script verification flags: Removing any flag(s) should not invalidate a valid transaction
            for (const auto& [name, flag] : mapFlagNames) {
                // Removing individual flags
                unsigned int flags = TrimFlags(~(verify_flags | flag));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags, txdata, txsigcheck_opts, strTest, /* expect_valid */ true)) {
                    BOOST_ERROR("Tx unexpectedly failed with flag " << name << " unset: " << strTest);
                }
                // Removing random combinations of flags
                flags = TrimFlags(~(verify_flags | (unsigned int)InsecureRandBits(mapFlagNames.size())));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags, txdata, txsigcheck_opts, strTest, /* expect_valid */ true)) {
                    BOOST_ERROR("Tx unexpectedly failed with random flags " << ToString(flags) << ": " << strTest);
                }
            }

            // Check that flags are maximal: transaction should fail if any unset flags are set.
            for (auto flags_excluding_one : ExcludeIndividualFlags(verify_flags)) {
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, ~flags_excluding_one, txdata, txsigcheck_opts, strTest, /* expect_valid */ false)) {
                    BOOST_ERROR("Too many flags unset: " << strTest);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(tx_invalid)
{
    // Read tests from test/data/tx_invalid.json
    UniValue tests = read_json(std::string(json_tests::tx_invalid, json_tests::tx_invalid + sizeof(json_tests::tx_invalid)));

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test[0].isArray())
        {
            if ((test.size() != 3 && test.size() != 4) || !test[1].isStr() || !test[2].isStr())
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::map<COutPoint, CScript> mapprevOutScriptPubKeys;
            std::map<COutPoint, std::pair<CAmount, uint32_t>> mapprevOutValues;
            UniValue inputs = test[0].get_array();
            bool fValid = true;
            for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
                const UniValue& input = inputs[inpIdx];
                if (!input.isArray()) {
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
                    mapprevOutValues[outpoint] = {vinput[3].get_int64(), 0};
                }
                if (vinput.size() >= 5)
                {
                    int64_t refheight = vinput[4].get_int64();
                    assert(refheight >= std::numeric_limits<uint32_t>::min());
                    assert(refheight <= std::numeric_limits<uint32_t>::max());
                    mapprevOutValues[outpoint].second = static_cast<uint32_t>(refheight);
                }
            }
            if (!fValid)
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::string transaction = test[1].get_str();
            CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION );
            CTransaction tx(deserialize, stream);
            // Tests imported from bitcoin with witness values use 0x00 for the
            // dummy value, which often results in a successful deserialization
            // with an empty input vector and a single output, and don't use up
            // the serialiation buffer. These tests should fail and be updated.
            BOOST_CHECK(stream.empty());

            TxValidationState state;
            if (!CheckTransaction(tx, state, Consensus::NONE) || state.IsInvalid()) {
                BOOST_CHECK_MESSAGE(test[2].get_str() == "BADTX", strTest);
                continue;
            }

            PrecomputedTransactionData txdata(tx);
            unsigned int verify_flags = ParseScriptFlags(test[2].get_str());
            TxSigCheckOpt txsigcheck_opts = TxSigCheckOpt::NONE;
            if (verify_flags & SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE) {
                txsigcheck_opts = TxSigCheckOpt::NO_LOCK_HEIGHT;
                verify_flags &= ~SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE;
            }

            // Check that the test gives a valid combination of flags (otherwise VerifyScript will throw). Don't edit the flags.
            if (verify_flags != FillFlags(verify_flags)) {
                BOOST_ERROR("Bad test flags: " << strTest);
            }

            ScriptError expected_err = SCRIPT_ERR_OK;
            if (test.size() == 4) {
                expected_err = ParseScriptError(test[3].get_str());
            }

            // Not using FillFlags() in the main test, in order to detect invalid verifyFlags combination
            BOOST_CHECK_MESSAGE(CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, verify_flags, txdata, txsigcheck_opts, strTest, /* expect_valid */ false, expected_err),
                                "Tx unexpectedly passed: " << strTest);

            // Backwards compatibility of script verification flags: Adding any flag(s) should not validate an invalid transaction
            for (const auto& [name, flag] : mapFlagNames) {
                unsigned int flags = FillFlags(verify_flags | flag);
                // Adding individual flags
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags, txdata, txsigcheck_opts, strTest, /* expect_valid */ false)) {
                    BOOST_ERROR("Tx unexpectedly passed with flag " << name << " set: " << strTest);
                }
                // Adding random combinations of flags
                flags = FillFlags(verify_flags | (unsigned int)InsecureRandBits(mapFlagNames.size()));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags, txdata, txsigcheck_opts, strTest, /* expect_valid */ false)) {
                    BOOST_ERROR("Tx unexpectedly passed with random flags " << name << ": " << strTest);
                }
            }

            // Check that flags are minimal: transaction should succeed if any set flags are unset.
            for (auto flags_excluding_one : ExcludeIndividualFlags(verify_flags)) {
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags_excluding_one, txdata, txsigcheck_opts, strTest, /* expect_valid */ true)) {
                    BOOST_ERROR("Too many flags set: " << strTest);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(basic_transaction_tests)
{
    // Random real transaction (b25458e2df302ff1ffaaa83969c22a3b94daba46b87f3c4f31eac153a1d9a31d)
    unsigned char ch[] = {0x02, 0x00, 0x00, 0x00, 0x01, 0x36, 0x0a, 0xc4, 0x73, 0x07, 0x05, 0xd8, 0xaa, 0x9e, 0x64, 0xdb, 0x8e, 0x92, 0x75, 0xe0, 0x1f, 0x9c, 0x63, 0xd1, 0x8c, 0x07, 0xba, 0xdd, 0x36, 0xde, 0x9f, 0x42, 0x67, 0x22, 0xb3, 0x94, 0x40, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x49, 0x30, 0x46, 0x02, 0x21, 0x00, 0xb3, 0xb7, 0x35, 0xd1, 0x69, 0xa8, 0xb9, 0x99, 0x7f, 0x27, 0x83, 0xc8, 0x3d, 0xe2, 0x17, 0x58, 0xe4, 0xf8, 0xd4, 0x41, 0x4d, 0x16, 0xe7, 0x75, 0x1e, 0xde, 0x63, 0x0c, 0x09, 0xaa, 0x01, 0x33, 0x02, 0x21, 0x00, 0xad, 0x5d, 0x77, 0xbb, 0xb2, 0xd8, 0xd8, 0xd0, 0x30, 0x6f, 0x17, 0xec, 0x12, 0x72, 0x48, 0x66, 0x57, 0x19, 0xfb, 0x17, 0x86, 0xab, 0x0c, 0xc3, 0x67, 0xee, 0x69, 0x2b, 0xb6, 0x5e, 0x74, 0x0b, 0x01, 0xff, 0xff, 0xff, 0xff, 0x02, 0x4f, 0x43, 0x62, 0x08, 0x05, 0x00, 0x00, 0x00, 0x19, 0x76, 0xa9, 0x14, 0xe6, 0x7f, 0xa0, 0xa2, 0xcd, 0x71, 0xab, 0x98, 0xa4, 0xfe, 0xb2, 0x36, 0xcf, 0x78, 0xed, 0xfb, 0xf0, 0x62, 0x1a, 0x54, 0x88, 0xac, 0xa6, 0xeb, 0x23, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x19, 0x76, 0xa9, 0x14, 0x90, 0x69, 0x45, 0x10, 0x72, 0x5f, 0x9e, 0x60, 0xe7, 0xc6, 0xf1, 0xb1, 0x5b, 0xe4, 0x09, 0x0e, 0x20, 0xba, 0xb2, 0x57, 0x88, 0xac, 0x00, 0x00, 0x00, 0x00, 0x44, 0x2f, 0x00, 0x00, 0x00};
    std::vector<unsigned char> vch(ch, ch + sizeof(ch) -1);
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    CMutableTransaction tx;
    stream >> tx;
    TxValidationState state;
    BOOST_CHECK_MESSAGE(CheckTransaction(CTransaction(tx), state, Consensus::NONE) && state.IsValid(), "Simple deserialized transaction should be valid.");

    // Check that duplicate txins fail
    tx.vin.push_back(tx.vin[0]);
    BOOST_CHECK_MESSAGE(!CheckTransaction(CTransaction(tx), state, Consensus::NONE) || !state.IsValid(), "Transaction with duplicate txins should be invalid.");
}

BOOST_AUTO_TEST_CASE(test_Get)
{
    FillableSigningProvider keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions =
        SetupDummyInputs(keystore, coins, {11*CENT, 50*CENT, 21*CENT, 22*CENT});

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

    BOOST_CHECK(AreInputsStandard(CTransaction(t1), coins, false));
    BOOST_CHECK_EQUAL(coins.GetValueIn(CTransaction(t1)), (50+21+22)*CENT);
}

static void CreateCreditAndSpend(const FillableSigningProvider& keystore, const CScript& outscript, CTransactionRef& output, CMutableTransaction& input, bool success = true)
{
    CMutableTransaction outputm;
    outputm.nVersion = 1;
    outputm.vin.resize(1);
    outputm.vin[0].prevout.SetNull();
    outputm.vin[0].scriptSig = CScript();
    outputm.vout.resize(1);
    outputm.vout[0].nValue = 1;
    outputm.vout[0].scriptPubKey = outscript;
    CDataStream ssout(SER_NETWORK, PROTOCOL_VERSION);
    ssout << outputm;
    ssout >> output;
    assert(output->vin.size() == 1);
    assert(output->vin[0] == outputm.vin[0]);
    assert(output->vout.size() == 1);
    assert(output->vout[0] == outputm.vout[0]);

    CMutableTransaction inputm;
    inputm.nVersion = 1;
    inputm.vin.resize(1);
    inputm.vin[0].prevout.hash = output->GetHash();
    inputm.vin[0].prevout.n = 0;
    inputm.vout.resize(1);
    inputm.vout[0].nValue = 1;
    inputm.vout[0].scriptPubKey = CScript();
    bool ret = SignSignature(keystore, *output, inputm, 0, SIGHASH_ALL);
    assert(ret == success);
    CDataStream ssin(SER_NETWORK, PROTOCOL_VERSION);
    ssin << inputm;
    ssin >> input;
    assert(input.vin.size() == 1);
    assert(input.vin[0] == inputm.vin[0]);
    assert(input.vout.size() == 1);
    assert(input.vout[0] == inputm.vout[0]);
    assert(input.vin[0].scriptWitness.stack == inputm.vin[0].scriptWitness.stack);
}

static void CheckWithFlag(const CTransactionRef& output, const CMutableTransaction& input, int flags, bool success)
{
    ScriptError error;
    CTransaction inputi(input);
    bool ret = VerifyScript(inputi.vin[0].scriptSig, output->vout[0].scriptPubKey, &inputi.vin[0].scriptWitness, flags, TransactionSignatureChecker(&inputi, 0, output->vout[0].nValue, output->lock_height, MissingDataBehavior::ASSERT_FAIL), &error);
    BOOST_CHECK_MESSAGE(ret == success, FormatScriptError(error));
}

static CScript PushAll(const std::vector<valtype>& values)
{
    CScript result;
    for (const valtype& v : values) {
        if (v.size() == 0) {
            result << OP_0;
        } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
            result << CScript::EncodeOP_N(v[0]);
        } else if (v.size() == 1 && v[0] == 0x81) {
            result << OP_1NEGATE;
        } else {
            result << v;
        }
    }
    return result;
}

static void ReplaceRedeemScript(CScript& script, const CScript& redeemScript)
{
    std::vector<valtype> stack;
    EvalScript(stack, script, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE);
    assert(stack.size() > 0);
    stack.back() = std::vector<unsigned char>(redeemScript.begin(), redeemScript.end());
    script = PushAll(stack);
}

BOOST_AUTO_TEST_CASE(test_big_witness_transaction)
{
    CMutableTransaction mtx;
    mtx.nVersion = 1;

    CKey key;
    key.MakeNewKey(true); // Need to use compressed keys in segwit or the signing will fail
    FillableSigningProvider keystore;
    BOOST_CHECK(keystore.AddKeyPubKey(key, key.GetPubKey()));
    CScript witscript = CScript() << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
    std::vector<unsigned char> innerscript(1, 0x00);
    innerscript.insert(innerscript.end(), witscript.begin(), witscript.end());
    keystore.AddWitnessV0Script(WitnessV0ScriptEntry(innerscript));
    CScript scriptPubKey = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, witscript));

    std::vector<int> sigHashes;
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

    CDataStream ssout(SER_NETWORK, PROTOCOL_VERSION);
    ssout << mtx;
    CTransaction tx(deserialize, ssout);

    // check all inputs concurrently, with the cache
    PrecomputedTransactionData txdata(tx);
    CCheckQueue<CScriptCheck> scriptcheckqueue(128);
    CCheckQueueControl<CScriptCheck> control(&scriptcheckqueue);

    scriptcheckqueue.StartWorkerThreads(20);

    std::vector<Coin> coins;
    for(uint32_t i = 0; i < mtx.vin.size(); i++) {
        Coin coin;
        coin.nHeight = 1;
        coin.fCoinBase = false;
        coin.out.nValue = 1000;
        coin.out.scriptPubKey = scriptPubKey;
        coins.emplace_back(std::move(coin));
    }

    for(uint32_t i = 0; i < mtx.vin.size(); i++) {
        std::vector<CScriptCheck> vChecks;
        const Coin& coin = coins[tx.vin[i].prevout.n];
        CScriptCheck check(coin.out, coin.refheight, tx, i, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false, &txdata);
        vChecks.push_back(CScriptCheck());
        check.swap(vChecks.back());
        control.Add(vChecks);
    }

    bool controlCheck = control.Wait();
    assert(controlCheck);
    scriptcheckqueue.StopWorkerThreads();
}

SignatureData CombineSignatures(const CMutableTransaction& input1, const CMutableTransaction& input2, const CTransactionRef tx)
{
    SignatureData sigdata;
    sigdata = DataFromTransaction(input1, 0, tx->vout[0], tx->lock_height);
    sigdata.MergeSignatureData(DataFromTransaction(input2, 0, tx->vout[0], tx->lock_height));
    ProduceSignature(DUMMY_SIGNING_PROVIDER, MutableTransactionSignatureCreator(&input1, 0, tx->vout[0].nValue, tx->lock_height, SIGHASH_ALL), tx->vout[0].scriptPubKey, sigdata);
    return sigdata;
}

BOOST_AUTO_TEST_CASE(test_witness)
{
    FillableSigningProvider keystore, keystore2;
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
    BOOST_CHECK(keystore.AddKeyPubKey(key1, pubkey1));
    BOOST_CHECK(keystore.AddKeyPubKey(key2, pubkey2));
    BOOST_CHECK(keystore.AddKeyPubKey(key1L, pubkey1L));
    BOOST_CHECK(keystore.AddKeyPubKey(key2L, pubkey2L));
    CScript scriptPubkey1, scriptPubkey2, scriptPubkey1L, scriptPubkey2L, scriptMulti;
    scriptPubkey1 << ToByteVector(pubkey1) << OP_CHECKSIG;
    scriptPubkey2 << ToByteVector(pubkey2) << OP_CHECKSIG;
    scriptPubkey1L << ToByteVector(pubkey1L) << OP_CHECKSIG;
    scriptPubkey2L << ToByteVector(pubkey2L) << OP_CHECKSIG;
    std::vector<CPubKey> oneandthree;
    oneandthree.push_back(pubkey1);
    oneandthree.push_back(pubkey3);
    scriptMulti = GetScriptForMultisig(2, oneandthree);
    BOOST_CHECK(keystore.AddCScript(scriptPubkey1));
    BOOST_CHECK(keystore.AddCScript(scriptPubkey2));
    BOOST_CHECK(keystore.AddCScript(scriptPubkey1L));
    BOOST_CHECK(keystore.AddCScript(scriptPubkey2L));
    BOOST_CHECK(keystore.AddCScript(scriptMulti));
    std::vector<unsigned char> vch = { 0 /* version */ };
    BOOST_CHECK(keystore.AddWitnessV0Script(WitnessV0ScriptEntry(vch)));
    vch.resize(1);
    vch.insert(vch.end(), scriptPubkey2.begin(), scriptPubkey2.end());
    BOOST_CHECK(keystore.AddWitnessV0Script(WitnessV0ScriptEntry(vch)));
    vch.resize(1);
    vch.insert(vch.end(), scriptPubkey1L.begin(), scriptPubkey1L.end());
    BOOST_CHECK(keystore.AddWitnessV0Script(WitnessV0ScriptEntry(vch)));
    vch.resize(1);
    vch.insert(vch.end(), scriptPubkey2L.begin(), scriptPubkey2L.end());
    BOOST_CHECK(keystore.AddWitnessV0Script(WitnessV0ScriptEntry(vch)));
    vch.resize(1);
    vch.insert(vch.end(), scriptMulti.begin(), scriptMulti.end());
    BOOST_CHECK(keystore.AddWitnessV0Script(WitnessV0ScriptEntry(vch)));
    CScript destination_script_1, destination_script_2, destination_script_1L, destination_script_2L, destination_script_multi;
    destination_script_1 = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, pubkey1));
    destination_script_2 = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, pubkey2));
    destination_script_1L = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, CScript() << ToByteVector(pubkey1L) << OP_CHECKSIG));
    destination_script_2L = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, CScript() << ToByteVector(pubkey2L) << OP_CHECKSIG));
    destination_script_multi = GetScriptForDestination(WitnessV0LongHash(0 /* version */, scriptMulti));
    BOOST_CHECK(keystore.AddCScript(destination_script_1));
    BOOST_CHECK(keystore.AddCScript(destination_script_2));
    BOOST_CHECK(keystore.AddCScript(destination_script_1L));
    BOOST_CHECK(keystore.AddCScript(destination_script_2L));
    BOOST_CHECK(keystore.AddCScript(destination_script_multi));
    BOOST_CHECK(keystore2.AddCScript(scriptMulti));
    BOOST_CHECK(keystore2.AddWitnessV0Script(WitnessV0ScriptEntry(vch /* scriptMulti */)));
    BOOST_CHECK(keystore2.AddCScript(destination_script_multi));
    BOOST_CHECK(keystore2.AddKeyPubKey(key3, pubkey3));

    CTransactionRef output1, output2;
    CMutableTransaction input1, input2;

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
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptPubkey1)), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptPubkey2)), output2, input2);
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
    CreateCreditAndSpend(keystore, destination_script_1, output1, input1);
    CreateCreditAndSpend(keystore, destination_script_2, output2, input2);
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
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptPubkey1L)), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptPubkey2L)), output2, input2);
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
    CreateCreditAndSpend(keystore, destination_script_1L, output1, input1, false);
    CreateCreditAndSpend(keystore, destination_script_2L, output2, input2, false);

    // Normal 2-of-2 multisig
    CreateCreditAndSpend(keystore, scriptMulti, output1, input1, false);
    CheckWithFlag(output1, input1, 0, false);
    CreateCreditAndSpend(keystore2, scriptMulti, output2, input2, false);
    CheckWithFlag(output2, input2, 0, false);
    BOOST_CHECK(*output1 == *output2);
    UpdateInput(input1.vin[0], CombineSignatures(input1, input2, output1));
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);

    // P2SH 2-of-2 multisig
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptMulti)), output1, input1, false);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, false);
    CreateCreditAndSpend(keystore2, GetScriptForDestination(ScriptHash(scriptMulti)), output2, input2, false);
    CheckWithFlag(output2, input2, 0, true);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH, false);
    BOOST_CHECK(*output1 == *output2);
    UpdateInput(input1.vin[0], CombineSignatures(input1, input2, output1));
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);

    // Witness 2-of-2 multisig
    CreateCreditAndSpend(keystore, destination_script_multi, output1, input1, false);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    CreateCreditAndSpend(keystore2, destination_script_multi, output2, input2, false);
    CheckWithFlag(output2, input2, 0, true);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    BOOST_CHECK(*output1 == *output2);
    UpdateInput(input1.vin[0], CombineSignatures(input1, input2, output1));
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
}

BOOST_AUTO_TEST_CASE(test_IsStandard)
{
    LOCK(cs_main);
    FillableSigningProvider keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions =
        SetupDummyInputs(keystore, coins, {11*CENT, 50*CENT, 21*CENT, 22*CENT});

    CMutableTransaction t;
    t.vin.resize(1);
    t.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t.vin[0].prevout.n = 1;
    t.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t.vout.resize(1);
    t.vout[0].nValue = 90*CENT;
    CKey key;
    key.MakeNewKey(true);
    t.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

    std::string reason;
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));

    // Check dust with default relay fee:
    CAmount nDustThreshold = 182 * dustRelayFee.GetFeePerK()/1000;
    BOOST_CHECK_EQUAL(nDustThreshold, 546);
    // dust:
    t.vout[0].nValue = nDustThreshold - 1;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "dust");
    // not dust:
    t.vout[0].nValue = nDustThreshold;
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));

    // Disallowed nVersion
    t.nVersion = -1;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "version");

    t.nVersion = 0;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "version");

    t.nVersion = 3;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "version");

    // Allowed nVersion
    t.nVersion = 1;
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));

    t.nVersion = 2;
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));

    // Check dust with odd relay fee to verify rounding:
    // nDustThreshold = 182 * 3702 / 1000
    dustRelayFee = CFeeRate(3702);
    // dust:
    t.vout[0].nValue = 674 - 1;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "dust");
    // not dust:
    t.vout[0].nValue = 674;
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));
    dustRelayFee = CFeeRate(DUST_RELAY_TX_FEE);

    t.vout[0].scriptPubKey = CScript() << OP_1;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "scriptpubkey");

    // MAX_OP_RETURN_RELAY-byte TxoutType::UNSPENDABLE (non-standard since removal of '-datacarrier')
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe554827");
    BOOST_CHECK_EQUAL(51, t.vout[0].scriptPubKey.size());
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));

    // MAX_OP_RETURN_RELAY+1-byte TxoutType::UNSPENDABLE (non-standard always)
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe55482719");
    BOOST_CHECK_EQUAL(51 + 1, t.vout[0].scriptPubKey.size());
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "scriptpubkey");

    // Data payload can be encoded in any way...
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("");
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("00") << ParseHex("01");
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    // OP_RESERVED *is* considered to be a PUSHDATA type opcode by IsPushOnly()!
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << OP_RESERVED << -1 << 0 << ParseHex("01") << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16;
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << 0 << ParseHex("01") << 2 << ParseHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));

    // ...so long as it only contains PUSHDATA's
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << OP_RETURN;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "scriptpubkey");

    // TxoutType::UNSPENDABLE: TX_NULL_DATA from bitcoin w/o PUSHDATA
    t.vout.resize(1);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));

    // Only one TxoutType::UNSPENDABLE permitted in all cases
    t.vout.resize(2);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[0].nValue = 0;
    t.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].nValue = 0;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "scriptpubkey");

    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "scriptpubkey");

    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "multi-op-return");

    // Check large scriptSig (non-standard if size is >1650 bytes)
    t.vout.resize(1);
    t.vout[0].nValue = MAX_MONEY;
    t.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
    // OP_PUSHDATA2 with len (3 bytes) + data (1647 bytes) = 1650 bytes
    t.vin[0].scriptSig = CScript() << std::vector<unsigned char>(1647, 0); // 1650
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));

    t.vin[0].scriptSig = CScript() << std::vector<unsigned char>(1648, 0); // 1651
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "scriptsig-size");

    // Check scriptSig format (non-standard if there are any other ops than just PUSHs)
    t.vin[0].scriptSig = CScript()
        << OP_TRUE << OP_0 << OP_1NEGATE << OP_16 // OP_n (single byte pushes: n = 1, 0, -1, 16)
        << std::vector<unsigned char>(75, 0)      // OP_PUSHx [...x bytes...]
        << std::vector<unsigned char>(235, 0)     // OP_PUSHDATA1 x [...x bytes...]
        << std::vector<unsigned char>(1234, 0)    // OP_PUSHDATA2 x [...x bytes...]
        << OP_9;
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));

    const std::vector<unsigned char> non_push_ops = { // arbitrary set of non-push operations
        OP_NOP, OP_VERIFY, OP_IF, OP_ROT, OP_3DUP, OP_SIZE, OP_EQUAL, OP_ADD, OP_SUB,
        OP_HASH256, OP_CODESEPARATOR, OP_CHECKSIG, OP_CHECKLOCKTIMEVERIFY };

    CScript::const_iterator pc = t.vin[0].scriptSig.begin();
    while (pc < t.vin[0].scriptSig.end()) {
        opcodetype opcode;
        CScript::const_iterator prev_pc = pc;
        t.vin[0].scriptSig.GetOp(pc, opcode); // advance to next op
        // for the sake of simplicity, we only replace single-byte push operations
        if (opcode >= 1 && opcode <= OP_PUSHDATA4)
            continue;

        int index = prev_pc - t.vin[0].scriptSig.begin();
        unsigned char orig_op = *prev_pc; // save op
        // replace current push-op with each non-push-op
        for (auto op : non_push_ops) {
            t.vin[0].scriptSig[index] = op;
            BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
            BOOST_CHECK_EQUAL(reason, "scriptsig-not-pushonly");
        }
        t.vin[0].scriptSig[index] = orig_op; // restore op
        BOOST_CHECK(IsStandardTx(CTransaction(t), reason));
    }

    // Check tx-size (non-standard if transaction weight is > MAX_STANDARD_TX_WEIGHT)
    t.vin.clear();
    t.vin.resize(2438); // size per input (empty scriptSig): 41 bytes
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << std::vector<unsigned char>(15, 0); // output size: 26 bytes
    // tx header:                16 bytes =>     64 vbytes
    // 2438 inputs: 2438*41 = 99958 bytes => 399832 vbytes
    //    1 output:              26 bytes =>    104 vbytes
    //                      ===============================
    //                                total: 400000 vbytes
    BOOST_CHECK_EQUAL(GetTransactionWeight(CTransaction(t)), 400000);
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "scriptpubkey");

    // increase output size by one byte, so we end up with 400004 vbytes
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << std::vector<unsigned char>(16, 0); // output size: 27 bytes
    BOOST_CHECK_EQUAL(GetTransactionWeight(CTransaction(t)), 400004);
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "tx-size");

    // Check bare multisig (standard if policy flag fIsBareMultisigStd is set)
    fIsBareMultisigStd = true;
    t.vout[0].scriptPubKey = GetScriptForMultisig(1, {key.GetPubKey()}); // simple 1-of-1
    t.vin.resize(1);
    t.vin[0].scriptSig = CScript() << std::vector<unsigned char>(65, 0);
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason));

    fIsBareMultisigStd = false;
    reason.clear();
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
    BOOST_CHECK_EQUAL(reason, "bare-multisig");
    fIsBareMultisigStd = DEFAULT_PERMIT_BAREMULTISIG;
}

BOOST_AUTO_TEST_SUITE_END()
