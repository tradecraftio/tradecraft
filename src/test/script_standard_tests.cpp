// Copyright (c) 2017-2021 The Bitcoin Core developers
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

#include <test/data/bip341_wallet_vectors.json.h>

#include <key.h>
#include <key_io.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <univalue.h>


BOOST_FIXTURE_TEST_SUITE(script_standard_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(dest_default_is_no_dest)
{
    CTxDestination dest;
    BOOST_CHECK(!IsValidDestination(dest));
}

BOOST_AUTO_TEST_CASE(script_standard_Solver_success)
{
    CKey keys[3];
    CPubKey pubkeys[3];
    for (int i = 0; i < 3; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CScript s;
    std::vector<std::vector<unsigned char> > solutions;

    // TxoutType::PUBKEY
    s.clear();
    s << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::PUBKEY);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(pubkeys[0]));

    // TxoutType::PUBKEYHASH
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::PUBKEYHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(pubkeys[0].GetID()));

    // TxoutType::SCRIPTHASH
    CScript redeemScript(s); // initialize with leftover P2PKH script
    s.clear();
    s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::SCRIPTHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(CScriptID(redeemScript)));

    // TxoutType::MULTISIG
    s.clear();
    s << OP_1 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::MULTISIG);
    BOOST_CHECK_EQUAL(solutions.size(), 4U);
    BOOST_CHECK(solutions[0] == std::vector<unsigned char>({1}));
    BOOST_CHECK(solutions[1] == ToByteVector(pubkeys[0]));
    BOOST_CHECK(solutions[2] == ToByteVector(pubkeys[1]));
    BOOST_CHECK(solutions[3] == std::vector<unsigned char>({2}));

    s.clear();
    s << OP_2 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        ToByteVector(pubkeys[2]) <<
        OP_3 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::MULTISIG);
    BOOST_CHECK_EQUAL(solutions.size(), 5U);
    BOOST_CHECK(solutions[0] == std::vector<unsigned char>({2}));
    BOOST_CHECK(solutions[1] == ToByteVector(pubkeys[0]));
    BOOST_CHECK(solutions[2] == ToByteVector(pubkeys[1]));
    BOOST_CHECK(solutions[3] == ToByteVector(pubkeys[2]));
    BOOST_CHECK(solutions[4] == std::vector<unsigned char>({3}));

    // TxoutType::NULL_DATA
    s.clear();
    s << OP_RETURN <<
        std::vector<unsigned char>({0}) <<
        std::vector<unsigned char>({75}) <<
        std::vector<unsigned char>({255});
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NULL_DATA);
    BOOST_CHECK_EQUAL(solutions.size(), 0U);

    // TxoutType::UNSPENDABLE
    s.clear();
    s << OP_RETURN;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::UNSPENDABLE);
    BOOST_CHECK_EQUAL(solutions.size(), 0U);

    // TxoutType::WITNESS_V0_LONGHASH
    CScript witnessScript_inner;
    witnessScript_inner << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    std::vector<unsigned char> witnessScript;
    witnessScript.push_back(0x00);
    witnessScript.insert(witnessScript.end(),
                         witnessScript_inner.begin(),
                         witnessScript_inner.end());

    WitnessV0LongHash long_hash;
    CHash256().Write(witnessScript).Finalize(long_hash);
    s.clear();
    s << OP_0 << ToByteVector(long_hash);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_V0_LONGHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(long_hash));

    // TxoutType::WITNESS_V0_SHORTHASH
    WitnessV0ShortHash short_hash;
    CRIPEMD160()
        .Write(long_hash.begin(), 32)
        .Finalize(short_hash.begin());
    s.clear();
    s << OP_0 << ToByteVector(short_hash);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_V0_SHORTHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(short_hash));

    // TxoutType::WITNESS_V1_TAPROOT
    s.clear();
    s << OP_1NEGATE << ToByteVector(uint256::ZERO);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_UNKNOWN);
    BOOST_CHECK_EQUAL(solutions.size(), 2U);
    BOOST_CHECK(solutions[0] == std::vector<unsigned char>{1});
    BOOST_CHECK(solutions[1] == ToByteVector(uint256::ZERO));

    // TxoutType::WITNESS_UNKNOWN
    s.clear();
    s << OP_16 << ToByteVector(uint256::ONE);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_UNKNOWN);
    BOOST_CHECK_EQUAL(solutions.size(), 2U);
    BOOST_CHECK(solutions[0] == std::vector<unsigned char>{17});
    BOOST_CHECK(solutions[1] == ToByteVector(uint256::ONE));

    // TxoutType::NONSTANDARD
    s.clear();
    s << OP_9 << OP_ADD << OP_11 << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);
}

BOOST_AUTO_TEST_CASE(script_standard_Solver_failure)
{
    CKey key;
    CPubKey pubkey;
    key.MakeNewKey(true);
    pubkey = key.GetPubKey();

    CScript s;
    std::vector<std::vector<unsigned char> > solutions;

    // TxoutType::PUBKEY with incorrectly sized pubkey
    s.clear();
    s << std::vector<unsigned char>(30, 0x01) << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::PUBKEYHASH with incorrectly sized key hash
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkey) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::SCRIPTHASH with incorrectly sized script hash
    s.clear();
    s << OP_HASH160 << std::vector<unsigned char>(21, 0x01) << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::MULTISIG 0/2
    s.clear();
    s << OP_0 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::MULTISIG 2/1
    s.clear();
    s << OP_2 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::MULTISIG n = 2 with 1 pubkey
    s.clear();
    s << OP_1 << ToByteVector(pubkey) << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::MULTISIG n = 1 with 0 pubkeys
    s.clear();
    s << OP_1 << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::NULL_DATA with other opcodes
    s.clear();
    s << OP_RETURN << std::vector<unsigned char>({75}) << OP_ADD;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::WITNESS_UNKNOWN with incorrect program size
    s.clear();
    s << OP_0 << std::vector<unsigned char>(19, 0x01);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_UNKNOWN);
}

BOOST_AUTO_TEST_CASE(script_standard_ExtractDestination)
{
    CKey key;
    CPubKey pubkey;
    key.MakeNewKey(true);
    pubkey = key.GetPubKey();

    CScript s;
    CTxDestination address;

    // TxoutType::PUBKEY
    s.clear();
    s << ToByteVector(pubkey) << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get<PKHash>(address) == PKHash(pubkey));

    // TxoutType::PUBKEYHASH
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkey.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get<PKHash>(address) == PKHash(pubkey));

    // TxoutType::SCRIPTHASH
    CScript redeemScript(s); // initialize with leftover P2PKH script
    s.clear();
    s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get<ScriptHash>(address) == ScriptHash(redeemScript));

    // TxoutType::MULTISIG
    s.clear();
    s << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK(!ExtractDestination(s, address));

    // TxoutType::NULL_DATA
    s.clear();
    s << OP_RETURN << std::vector<unsigned char>({75});
    BOOST_CHECK(!ExtractDestination(s, address));

    // TxoutType::UNSPENDABLE
    s.clear();
    s << OP_RETURN;
    BOOST_CHECK(!ExtractDestination(s, address));

    // TxoutType::WITNESS_V0_LONGHASH
    unsigned char prefix = 0x00;
    WitnessV0LongHash long_hash;
    CHash256().Write({&prefix, 1}).Write(redeemScript).Finalize(long_hash);
    s.clear();
    s << OP_0 << ToByteVector(long_hash);
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get_if<WitnessV0LongHash>(&address) && *std::get_if<WitnessV0LongHash>(&address) == long_hash);

    // TxoutType::WITNESS_V0_SHORTHASH
    WitnessV0ShortHash short_hash;
    CRIPEMD160()
        .Write(long_hash.begin(),
               long_hash.size())
        .Finalize(short_hash.begin());
    s.clear();
    s << OP_0 << ToByteVector(short_hash);
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get_if<WitnessV0ShortHash>(&address) && *std::get_if<WitnessV0ShortHash>(&address) == short_hash);

    // TxoutType::WITNESS_UNKNOWN with unknown version
    s.clear();
    s << OP_1 << ToByteVector(pubkey);
    BOOST_CHECK(ExtractDestination(s, address));
    WitnessUnknown unk;
    unk.length = 33;
    unk.version = 2;
    std::copy(pubkey.begin(), pubkey.end(), unk.program);
    BOOST_CHECK(std::get<WitnessUnknown>(address) == unk);
}

BOOST_AUTO_TEST_CASE(script_standard_GetScriptFor_)
{
    CKey keys[3];
    CPubKey pubkeys[3];
    for (int i = 0; i < 3; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CScript expected, result;

    // PKHash
    expected.clear();
    expected << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    result = GetScriptForDestination(PKHash(pubkeys[0]));
    BOOST_CHECK(result == expected);

    // CScriptID
    CScript redeemScript(result);
    expected.clear();
    expected << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    result = GetScriptForDestination(ScriptHash(redeemScript));
    BOOST_CHECK(result == expected);

    // CNoDestination
    expected.clear();
    result = GetScriptForDestination(CNoDestination());
    BOOST_CHECK(result == expected);

    // GetScriptForRawPubKey
    expected.clear();
    expected << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    result = GetScriptForRawPubKey(pubkeys[0]);
    BOOST_CHECK(result == expected);

    // GetScriptForMultisig
    expected.clear();
    expected << OP_2 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        ToByteVector(pubkeys[2]) <<
        OP_3 << OP_CHECKMULTISIG;
    result = GetScriptForMultisig(2, std::vector<CPubKey>(pubkeys, pubkeys + 3));
    BOOST_CHECK(result == expected);
}

BOOST_AUTO_TEST_SUITE_END()
