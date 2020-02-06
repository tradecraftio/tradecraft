// Copyright (c) 2017-2019 The Bitcoin Core developers
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
#include <script/script.h>
#include <script/standard.h>
#include <test/setup_common.h>
#include <wallet/ismine.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>


BOOST_FIXTURE_TEST_SUITE(ismine_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ismine_standard)
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
    std::unique_ptr<interfaces::Chain> chain = interfaces::MakeChain();

    CScript scriptPubKey;
    isminetype result;

    // P2PK compressed
    {
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
        scriptPubKey = GetScriptForDestination(PKHash(pubkeys[0]));

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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
        scriptPubKey = GetScriptForDestination(PKHash(uncompressedPubkey));

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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);

        CScript redeemScript = GetScriptForDestination(PKHash(pubkeys[0]));
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));

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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);

        CScript redeemscript_inner = GetScriptForDestination(PKHash(pubkeys[0]));
        CScript redeemscript = GetScriptForDestination(ScriptHash(redeemscript_inner));
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemscript));

        BOOST_CHECK(keystore.AddCScript(redeemscript));
        BOOST_CHECK(keystore.AddCScript(redeemscript_inner));
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // (P2PKH inside) P2SH inside P2WSH (invalid)
    {
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);

        CScript redeemscript = GetScriptForDestination(PKHash(pubkeys[0]));
        CScript witnessscript = GetScriptForDestination(ScriptHash(redeemscript));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.AddCScript(witnessscript));
        BOOST_CHECK(keystore.AddCScript(redeemscript));
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, witnessscript));
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // P2WPKH inside P2WSH (invalid)
    {
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);

        CScript p2pk = GetScriptForRawPubKey(pubkeys[0]);
        CScript witnessscript = GetScriptForDestination(WitnessV0LongHash(0 /* version */, p2pk));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.AddCScript(witnessscript));
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.AddKey(keys[0]));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        witnessscript = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, p2pk));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));
        keystore.AddCScript(witnessscript);
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, witnessscript));
        keystore.AddCScript(scriptPubKey);
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // (P2PKH inside) P2WSH inside P2WSH (invalid)
    {
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);

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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
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
        BOOST_CHECK(keystore.AddCScript(scriptPubKey));
        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2WSH w/ P2PK uncompressed
    {
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);

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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
        BOOST_CHECK(keystore.AddKey(uncompressedKey));
        BOOST_CHECK(keystore.AddKey(keys[1]));

        CScript redeemScript = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));

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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);

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
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_RETURN << ToByteVector(pubkeys[0]);

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // witness unspendable
    {
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(ParseHex("aabb"));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // witness unknown
    {
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_16 << ToByteVector(ParseHex("aabb"));

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // Nonstandard
    {
        CWallet keystore(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(keystore.cs_wallet);
        BOOST_CHECK(keystore.AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_9 << OP_ADD << OP_11 << OP_EQUAL;

        result = IsMine(keystore, scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }
}

BOOST_AUTO_TEST_SUITE_END()
