// Copyright (c) 2017-2018 The Bitcoin Core developers
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

#include <key.h>
#include <keystore.h>
#include <script/ismine.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/standard.h>
#include <test/test_freicoin.h>

#include <boost/test/unit_test.hpp>


BOOST_FIXTURE_TEST_SUITE(script_standard_tests, BasicTestingSetup)

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

    // TX_PUBKEY
    s.clear();
    s << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_PUBKEY);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(pubkeys[0]));

    // TX_PUBKEYHASH
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_PUBKEYHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(pubkeys[0].GetID()));

    // TX_SCRIPTHASH
    CScript redeemScript(s); // initialize with leftover P2PKH script
    s.clear();
    s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_SCRIPTHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(CScriptID(redeemScript)));

    // TX_MULTISIG
    s.clear();
    s << OP_1 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_MULTISIG);
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
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_MULTISIG);
    BOOST_CHECK_EQUAL(solutions.size(), 5U);
    BOOST_CHECK(solutions[0] == std::vector<unsigned char>({2}));
    BOOST_CHECK(solutions[1] == ToByteVector(pubkeys[0]));
    BOOST_CHECK(solutions[2] == ToByteVector(pubkeys[1]));
    BOOST_CHECK(solutions[3] == ToByteVector(pubkeys[2]));
    BOOST_CHECK(solutions[4] == std::vector<unsigned char>({3}));

    // TX_UNSPENDABLE
    s.clear();
    s << OP_RETURN;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_UNSPENDABLE);
    BOOST_CHECK_EQUAL(solutions.size(), 0U);

    // TX_WITNESS_V0_LONGHASH
    CScript witnessScript_inner;
    witnessScript_inner << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    std::vector<unsigned char> witnessScript;
    witnessScript.push_back(0x00);
    witnessScript.insert(witnessScript.end(),
                         witnessScript_inner.begin(),
                         witnessScript_inner.end());

    WitnessV0LongHash long_hash;
    CHash256()
        .Write(&witnessScript[0],
               witnessScript.size())
        .Finalize(long_hash.begin());
    s.clear();
    s << OP_0 << ToByteVector(long_hash);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_WITNESS_V0_LONGHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(long_hash));

    // TX_WITNESS_V0_SHORTHASH
    WitnessV0ShortHash short_hash;
    CRIPEMD160()
        .Write(long_hash.begin(), 32)
        .Finalize(short_hash.begin());
    s.clear();
    s << OP_0 << ToByteVector(short_hash);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_WITNESS_V0_SHORTHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(short_hash));

    // TX_NONSTANDARD
    s.clear();
    s << OP_9 << OP_ADD << OP_11 << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);
}

BOOST_AUTO_TEST_CASE(script_standard_Solver_failure)
{
    CKey key;
    CPubKey pubkey;
    key.MakeNewKey(true);
    pubkey = key.GetPubKey();

    CScript s;
    std::vector<std::vector<unsigned char> > solutions;

    // TX_PUBKEY with incorrectly sized pubkey
    s.clear();
    s << std::vector<unsigned char>(30, 0x01) << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);

    // TX_PUBKEYHASH with incorrectly sized key hash
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkey) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);

    // TX_SCRIPTHASH with incorrectly sized script hash
    s.clear();
    s << OP_HASH160 << std::vector<unsigned char>(21, 0x01) << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);

    // TX_MULTISIG 0/2
    s.clear();
    s << OP_0 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);

    // TX_MULTISIG 2/1
    s.clear();
    s << OP_2 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);

    // TX_MULTISIG n = 2 with 1 pubkey
    s.clear();
    s << OP_1 << ToByteVector(pubkey) << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);

    // TX_MULTISIG n = 1 with 0 pubkeys
    s.clear();
    s << OP_1 << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);

    // TX_UNSPENDABLE with other opcodes
    s.clear();
    s << OP_RETURN << std::vector<unsigned char>({75}) << OP_ADD;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);

    // TX_WITNESS with incorrect program size
    s.clear();
    s << OP_0 << std::vector<unsigned char>(19, 0x01);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TX_NONSTANDARD);
}

BOOST_AUTO_TEST_CASE(script_standard_ExtractDestination)
{
    CKey key;
    CPubKey pubkey;
    key.MakeNewKey(true);
    pubkey = key.GetPubKey();

    CScript s;
    CTxDestination address;

    // TX_PUBKEY
    s.clear();
    s << ToByteVector(pubkey) << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(boost::get<CKeyID>(&address) &&
                *boost::get<CKeyID>(&address) == pubkey.GetID());

    // TX_PUBKEYHASH
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkey.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(boost::get<CKeyID>(&address) &&
                *boost::get<CKeyID>(&address) == pubkey.GetID());

    // TX_SCRIPTHASH
    CScript redeemScript(s); // initialize with leftover P2PKH script
    s.clear();
    s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(boost::get<CScriptID>(&address) &&
                *boost::get<CScriptID>(&address) == CScriptID(redeemScript));

    // TX_MULTISIG
    s.clear();
    s << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK(!ExtractDestination(s, address));

    // TX_UNSPENDABLE
    s.clear();
    s << OP_RETURN << std::vector<unsigned char>({75});
    BOOST_CHECK(!ExtractDestination(s, address));

    // TX_WITNESS_V0_LONGHASH
    unsigned char prefix = 0x00;
    WitnessV0LongHash long_hash;
    CHash256()
        .Write(&prefix, 1)
        .Write(redeemScript.data(),
               redeemScript.size())
        .Finalize(long_hash.begin());
    s.clear();
    s << OP_0 << ToByteVector(long_hash);
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(boost::get<WitnessV0LongHash>(&address) && *boost::get<WitnessV0LongHash>(&address) == long_hash);

    // TX_WITNESS_V0_SHORTHASH
    WitnessV0ShortHash short_hash;
    CRIPEMD160()
        .Write(long_hash.begin(),
               long_hash.size())
        .Finalize(short_hash.begin());
    s.clear();
    s << OP_0 << ToByteVector(short_hash);
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(boost::get<WitnessV0ShortHash>(&address) && *boost::get<WitnessV0ShortHash>(&address) == short_hash);

    // TX_WITNESS with unknown version
    s.clear();
    s << OP_1 << ToByteVector(pubkey);
    BOOST_CHECK(ExtractDestination(s, address));
    WitnessUnknown unk;
    unk.length = 33;
    unk.version = 2;
    std::copy(pubkey.begin(), pubkey.end(), unk.program);
    BOOST_CHECK(boost::get<WitnessUnknown>(&address) && *boost::get<WitnessUnknown>(&address) == unk);
}

BOOST_AUTO_TEST_CASE(script_standard_ExtractDestinations)
{
    CKey keys[3];
    CPubKey pubkeys[3];
    for (int i = 0; i < 3; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CScript s;
    txnouttype whichType;
    std::vector<CTxDestination> addresses;
    int nRequired;

    // TX_PUBKEY
    s.clear();
    s << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestinations(s, whichType, addresses, nRequired));
    BOOST_CHECK_EQUAL(whichType, TX_PUBKEY);
    BOOST_CHECK_EQUAL(addresses.size(), 1U);
    BOOST_CHECK_EQUAL(nRequired, 1);
    BOOST_CHECK(boost::get<CKeyID>(&addresses[0]) &&
                *boost::get<CKeyID>(&addresses[0]) == pubkeys[0].GetID());

    // TX_PUBKEYHASH
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestinations(s, whichType, addresses, nRequired));
    BOOST_CHECK_EQUAL(whichType, TX_PUBKEYHASH);
    BOOST_CHECK_EQUAL(addresses.size(), 1U);
    BOOST_CHECK_EQUAL(nRequired, 1);
    BOOST_CHECK(boost::get<CKeyID>(&addresses[0]) &&
                *boost::get<CKeyID>(&addresses[0]) == pubkeys[0].GetID());

    // TX_SCRIPTHASH
    CScript redeemScript(s); // initialize with leftover P2PKH script
    s.clear();
    s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    BOOST_CHECK(ExtractDestinations(s, whichType, addresses, nRequired));
    BOOST_CHECK_EQUAL(whichType, TX_SCRIPTHASH);
    BOOST_CHECK_EQUAL(addresses.size(), 1U);
    BOOST_CHECK_EQUAL(nRequired, 1);
    BOOST_CHECK(boost::get<CScriptID>(&addresses[0]) &&
                *boost::get<CScriptID>(&addresses[0]) == CScriptID(redeemScript));

    // TX_MULTISIG
    s.clear();
    s << OP_2 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK(ExtractDestinations(s, whichType, addresses, nRequired));
    BOOST_CHECK_EQUAL(whichType, TX_MULTISIG);
    BOOST_CHECK_EQUAL(addresses.size(), 2U);
    BOOST_CHECK_EQUAL(nRequired, 2);
    BOOST_CHECK(boost::get<CKeyID>(&addresses[0]) &&
                *boost::get<CKeyID>(&addresses[0]) == pubkeys[0].GetID());
    BOOST_CHECK(boost::get<CKeyID>(&addresses[1]) &&
                *boost::get<CKeyID>(&addresses[1]) == pubkeys[1].GetID());

    // TX_UNSPENDABLE
    s.clear();
    s << OP_RETURN << std::vector<unsigned char>({75});
    BOOST_CHECK(!ExtractDestinations(s, whichType, addresses, nRequired));
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

    // CKeyID
    expected.clear();
    expected << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    result = GetScriptForDestination(pubkeys[0].GetID());
    BOOST_CHECK(result == expected);

    // CScriptID
    CScript redeemScript(result);
    expected.clear();
    expected << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    result = GetScriptForDestination(CScriptID(redeemScript));
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

    // GetScriptForWitness
    CScript witnessScript;

    witnessScript << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    WitnessV0LongHash long_hash;
    unsigned char prefix = 0x00;
    CHash256()
        .Write(&prefix, 1)
        .Write(&witnessScript[0], witnessScript.size())
        .Finalize(long_hash.begin());
    WitnessV0ShortHash short_hash;
    CRIPEMD160()
        .Write(long_hash.begin(), 32)
        .Finalize(short_hash.begin());
    expected.clear();
    expected << OP_0 << ToByteVector(short_hash);
    result = GetScriptForWitness(witnessScript);
    BOOST_CHECK(result == expected);

    witnessScript.clear();
    witnessScript << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

    const unsigned char zero = 0;
    CHash256()
        .Write(&zero, 1)
        .Write(&witnessScript[0], witnessScript.size())
        .Finalize(long_hash.begin());
    CRIPEMD160()
        .Write(long_hash.begin(), 32)
        .Finalize(short_hash.begin());

    expected.clear();
    expected << OP_0 << ToByteVector(short_hash);
    result = GetScriptForWitness(witnessScript);
    BOOST_CHECK(result == expected);

    witnessScript.clear();
    witnessScript << OP_1 << ToByteVector(pubkeys[0]) << OP_1 << OP_CHECKMULTISIG;

    CHash256()
        .Write(&zero, 1)
        .Write(witnessScript.data(), witnessScript.size())
        .Finalize(long_hash.begin());

    expected.clear();
    expected << OP_0 << ToByteVector(long_hash);
    result = GetScriptForWitness(witnessScript);
    BOOST_CHECK(result == expected);
}

BOOST_AUTO_TEST_CASE(script_standard_IsMine)
{
    CKey keys[2];
    CPubKey pubkeys[2];
    for (int i = 0; i < 2; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CKey uncompressedKey;
    uncompressedKey.MakeNewKey(false);
    CPubKey uncompressedPubkey = uncompressedKey.GetPubKey();

    CScript scriptPubKey;
    isminetype result;

    // P2PK compressed
    {
        CBasicKeyStore keystore;
        scriptPubKey = GetScriptForRawPubKey(pubkeys[0]);

        // Keystore does not have key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PK uncompressed
    {
        CBasicKeyStore keystore;
        scriptPubKey = GetScriptForRawPubKey(uncompressedPubkey);

        // Keystore does not have key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key
        BOOST_CHECK(keystore.AddKey(uncompressedKey));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PKH compressed
    {
        CBasicKeyStore keystore;
        scriptPubKey = GetScriptForDestination(pubkeys[0].GetID());

        // Keystore does not have key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PKH uncompressed
    {
        CBasicKeyStore keystore;
        scriptPubKey = GetScriptForDestination(uncompressedPubkey.GetID());

        // Keystore does not have key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key
        BOOST_CHECK(keystore.AddKey(uncompressedKey));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2SH
    {
        CBasicKeyStore keystore;

        CScript redeemScript = GetScriptForDestination(pubkeys[0].GetID());
        scriptPubKey = GetScriptForDestination(CScriptID(redeemScript));

        // Keystore does not have redeemScript or key
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has redeemScript but no key
        BOOST_CHECK(keystore.AddCScript(redeemScript));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has redeemScript and key
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // (P2PKH inside) P2SH inside P2SH (invalid)
    {
        CBasicKeyStore keystore;

        CScript redeemscript_inner = GetScriptForDestination(pubkeys[0].GetID());
        CScript redeemscript = GetScriptForDestination(CScriptID(redeemscript_inner));
        scriptPubKey = GetScriptForDestination(CScriptID(redeemscript));

        BOOST_CHECK(keystore.AddCScript(redeemscript));
        BOOST_CHECK(keystore.AddCScript(redeemscript_inner));
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // (P2PKH inside) P2SH inside P2WSH (invalid)
    {
        CBasicKeyStore keystore;

        CScript redeemscript = GetScriptForDestination(pubkeys[0].GetID());
        CScript witnessscript = GetScriptForDestination(CScriptID(redeemscript));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.AddCScript(witnessscript));
        BOOST_CHECK(keystore.AddCScript(redeemscript));
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript));
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // P2WPKH inside P2WSH (invalid)
    {
        CBasicKeyStore keystore;

        CScript p2pk = GetScriptForRawPubKey(pubkeys[0]);
        CScript witnessscript = GetScriptForDestination(WitnessV0LongHash(0 /* version */, p2pk));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.AddCScript(witnessscript));
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript));
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        witnessscript = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, p2pk));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash((unsigned char)0, witnessscript));
        keystore.AddCScript(witnessscript);
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript));
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // (P2PKH inside) P2WSH inside P2WSH (invalid)
    {
        CBasicKeyStore keystore;

        CScript witnessscript_inner = GetScriptForRawPubKey(pubkeys[0]);
        CScript witnessscript = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript_inner));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.AddCScript(witnessscript_inner));
        BOOST_CHECK(keystore.AddCScript(witnessscript));
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript));
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        witnessscript = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript_inner));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash((unsigned char)0, witnessscript));
        keystore.AddCScript(witnessscript);
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript));
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // P2WSH w/ P2PK compressed
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(keys[0]));

        CScript witscript_inner;
        witscript_inner << ToByteVector(pubkeys[0]) << OP_CHECKSIG;

        std::vector<unsigned char> witscript;
        witscript.push_back(0x00);
        witscript.insert(witscript.end(),
                         witscript_inner.begin(),
                         witscript_inner.end());

        uint256 long_hash;
        CHash256()
            .Write(&witscript[0], witscript.size())
            .Finalize(long_hash.begin());
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        // Keystore has key and witness script
        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2WSH w/ P2PK uncompressed
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(uncompressedKey));

        CScript witscript_inner;
        witscript_inner << ToByteVector(uncompressedPubkey) << OP_CHECKSIG;

        std::vector<unsigned char> witscript;
        witscript.push_back(0x00);
        witscript.insert(witscript.end(),
                         witscript_inner.begin(),
                         witscript_inner.end());

        uint256 long_hash;
        CHash256()
            .Write(&witscript[0], witscript.size())
            .Finalize(long_hash.begin());
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        // Keystore has key, but no witness script
        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key and witness script
        WitnessV0ScriptEntry entry(witscript);
        BOOST_CHECK(keystore.AddWitnessV0Script(entry));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // scriptPubKey multisig
    {
        CBasicKeyStore keystore;

        scriptPubKey = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});

        // Keystore does not have any keys
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has 1/2 keys
        BOOST_CHECK(keystore.AddKey(uncompressedKey));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has 2/2 keys
        BOOST_CHECK(keystore.AddKey(keys[1]));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has 2/2 keys and the script
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // P2SH multisig
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(uncompressedKey));
        BOOST_CHECK(keystore.AddKey(keys[1]));

        CScript redeemScript = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});
        scriptPubKey = GetScriptForDestination(CScriptID(redeemScript));

        // Keystore has no redeemScript
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has redeemScript
        BOOST_CHECK(keystore.AddCScript(redeemScript));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2WSH multisig with compressed keys
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(keys[0]));
        BOOST_CHECK(keystore.AddKey(keys[1]));

        CScript witnessScript_inner = GetScriptForMultisig(2, {pubkeys[0], pubkeys[1]});

        std::vector<unsigned char> witnessScript;
        witnessScript.push_back(0x00);
        witnessScript.insert(witnessScript.end(),
                             witnessScript_inner.begin(),
                             witnessScript_inner.end());

        uint256 long_hash;
        CHash256()
            .Write(&witnessScript[0], witnessScript.size())
            .Finalize(long_hash.begin());
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        // Keystore has keys, but no witnessScript
        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Knowing the inner witness script is insufficient
        BOOST_CHECK(keystore.AddCScript(witnessScript_inner));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has keys & witnessScript
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.AddWitnessV0Script(entry));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2WSH multisig with uncompressed key
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(uncompressedKey));
        BOOST_CHECK(keystore.AddKey(keys[1]));

        CScript witnessScript_inner = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});

        std::vector<unsigned char> witnessScript;
        witnessScript.push_back(0x00);
        witnessScript.insert(witnessScript.end(),
                             witnessScript_inner.begin(),
                             witnessScript_inner.end());

        uint256 long_hash;
        CHash256()
            .Write(&witnessScript[0], witnessScript.size())
            .Finalize(long_hash.begin());
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        // Keystore has keys, but no witnessScript
        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Knowing the inner witness script is insufficient
        BOOST_CHECK(keystore.AddCScript(witnessScript_inner));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has keys & witnessScript
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.AddWitnessV0Script(entry));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // P2WSH multisig wrapped in P2SH
    {
        CBasicKeyStore keystore;

        CScript witnessScript_inner = GetScriptForMultisig(2, {pubkeys[0], pubkeys[1]});

        std::vector<unsigned char> witnessScript;
        witnessScript.push_back(0x00);
        witnessScript.insert(witnessScript.end(),
                             witnessScript_inner.begin(),
                             witnessScript_inner.end());

        uint256 long_hash;
        CHash256()
            .Write(&witnessScript[0], witnessScript.size())
            .Finalize(long_hash.begin());
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        CScript redeemScript;
        redeemScript << OP_0 << ToByteVector(long_hash);

        // Keystore has no witnessScript, P2SH redeemScript, or keys
        scriptPubKey.clear();
        scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has witnessScript and P2SH redeemScript, but no keys
        BOOST_CHECK(keystore.AddCScript(redeemScript));
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.AddWitnessV0Script(entry));

        scriptPubKey.clear();
        scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has keys, witnessScript, P2SH redeemScript
        BOOST_CHECK(keystore.AddKey(keys[0]));
        BOOST_CHECK(keystore.AddKey(keys[1]));

        scriptPubKey.clear();
        scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // OP_RETURN
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_RETURN << ToByteVector(pubkeys[0]);

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // witness unspendable
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(ParseHex("aabb"));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // witness unknown
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_16 << ToByteVector(ParseHex("aabb"));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // Nonstandard
    {
        CBasicKeyStore keystore;
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_9 << OP_ADD << OP_11 << OP_EQUAL;

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }
}

BOOST_AUTO_TEST_SUITE_END()
