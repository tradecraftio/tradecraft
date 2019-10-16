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

#include <key.h>
#include <node/context.h>
#include <script/script.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <wallet/ismine.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>


namespace wallet {
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
    std::unique_ptr<interfaces::Chain>& chain = m_node.chain;

    CScript scriptPubKey;
    isminetype result;

    // P2PK compressed
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        scriptPubKey = GetScriptForRawPubKey(pubkeys[0]);

        // Keystore does not have key
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has key
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2PK uncompressed
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        scriptPubKey = GetScriptForRawPubKey(uncompressedPubkey);

        // Keystore does not have key
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has key
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2PKH compressed
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        scriptPubKey = GetScriptForDestination(PKHash(pubkeys[0]));

        // Keystore does not have key
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has key
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2PKH uncompressed
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        scriptPubKey = GetScriptForDestination(PKHash(uncompressedPubkey));

        // Keystore does not have key
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has key
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2SH
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript redeemScript = GetScriptForDestination(PKHash(pubkeys[0]));
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));

        // Keystore does not have redeemScript or key
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has redeemScript but no key
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(redeemScript));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has redeemScript and key
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // (P2PKH inside) P2SH inside P2SH (invalid)
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript redeemscript_inner = GetScriptForDestination(PKHash(pubkeys[0]));
        CScript redeemscript = GetScriptForDestination(ScriptHash(redeemscript_inner));
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemscript));

        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(redeemscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(redeemscript_inner));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // (P2PKH inside) P2SH inside P2WSH (invalid)
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript redeemscript = GetScriptForDestination(PKHash(pubkeys[0]));
        CScript witnessscript = GetScriptForDestination(ScriptHash(redeemscript));
        scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(redeemscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // P2WPKH inside P2WSH (invalid)
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript witnessscript = GetScriptForDestination(WitnessV0KeyHash(pubkeys[0]));
        scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // (P2PKH inside) P2WSH inside P2WSH (invalid)
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript witnessscript_inner = GetScriptForDestination(PKHash(pubkeys[0]));
        CScript witnessscript = GetScriptForDestination(WitnessV0ScriptHash(0 /* version */, witnessscript_inner));
        scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript_inner));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // P2WPKH compressed
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));

        scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(pubkeys[0]));

        // Keystore implicitly has key and P2SH redeemScript
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2WPKH uncompressed
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));

        scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(uncompressedPubkey));

        // Keystore has key, but no P2SH redeemScript
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has key and P2SH redeemScript
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // scriptPubKey multisig
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        scriptPubKey = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});

        // Keystore does not have any keys
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has 1/2 keys
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));

        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has 2/2 keys
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[1]));

        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has 2/2 keys and the script
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));

        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // P2SH multisig
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[1]));

        CScript redeemScript = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));

        // Keystore has no redeemScript
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has redeemScript
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(redeemScript));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2WSH multisig with compressed keys
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[1]));

        CScript witnessScript_inner = GetScriptForMultisig(2, {pubkeys[0], pubkeys[1]});

        std::vector<unsigned char> witnessScript;
        witnessScript.push_back(0x00);
        witnessScript.insert(witnessScript.end(),
                             witnessScript_inner.begin(),
                             witnessScript_inner.end());

        uint256 scriptHash;
        CSHA256().Write(&witnessScript[0], witnessScript.size())
            .Finalize(scriptHash.begin());

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(scriptHash);

        // Keystore has keys, but no witnessScript
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Knowing the inner witness script is insufficient
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessScript_inner));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has keys & witnessScript
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddWitnessV0Script(entry));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2WSH multisig with uncompressed key
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[1]));

        CScript witnessScript_inner = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});

        std::vector<unsigned char> witnessScript;
        witnessScript.push_back(0x00);
        witnessScript.insert(witnessScript.end(),
                             witnessScript_inner.begin(),
                             witnessScript_inner.end());

        uint256 scriptHash;
        CSHA256().Write(&witnessScript[0], witnessScript.size())
            .Finalize(scriptHash.begin());

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(scriptHash);

        // Keystore has keys, but no witnessScript
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Knowing the inner witness script is insufficient
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessScript_inner));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has keys & witnessScript
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddWitnessV0Script(entry));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        // FIXME: The following returns a result, even though the
        //        script is not spendable (as it uses an uncompressed
        //        key in a segwit script).  This is because the adding
        //        of the WitnessV0ScriptEntry is not checked.  It is
        //        unclear if this should be changed.
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2WSH multisig wrapped in P2SH
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript witnessScript_inner = GetScriptForMultisig(2, {pubkeys[0], pubkeys[1]});

        std::vector<unsigned char> witnessScript;
        witnessScript.push_back(0x00);
        witnessScript.insert(witnessScript.end(),
                             witnessScript_inner.begin(),
                             witnessScript_inner.end());

        uint256 scriptHash;
        CSHA256().Write(&witnessScript[0], witnessScript.size())
            .Finalize(scriptHash.begin());

        CScript redeemScript;
        redeemScript << OP_0 << ToByteVector(scriptHash);

        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));

        // Keystore has no witnessScript, P2SH redeemScript, or keys
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has witnessScript and P2SH redeemScript, but no keys
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(redeemScript));
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddWitnessV0Script(entry));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has keys, witnessScript, P2SH redeemScript
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[1]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // OP_RETURN
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_RETURN << ToByteVector(pubkeys[0]);

        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // witness unspendable
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(ParseHex("aabb"));

        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // witness unknown
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_16 << ToByteVector(ParseHex("aabb"));

        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // Nonstandard
    {
        CWallet keystore(chain.get(), "", m_args, CreateDummyWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));

        scriptPubKey.clear();
        scriptPubKey << OP_9 << OP_ADD << OP_11 << OP_EQUAL;

        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
