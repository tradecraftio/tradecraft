// Copyright (c) 2017-2022 The Bitcoin Core developers
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

#include <key.h>
#include <key_io.h>
#include <node/context.h>
#include <script/script.h>
#include <script/solver.h>
#include <script/signingprovider.h>
#include <test/util/setup_common.h>
#include <wallet/types.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <boost/test/unit_test.hpp>


namespace wallet {
BOOST_FIXTURE_TEST_SUITE(ismine_tests, BasicTestingSetup)

wallet::ScriptPubKeyMan* CreateDescriptor(CWallet& keystore, const std::string& desc_str, const bool success)
{
    keystore.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);

    FlatSigningProvider keys;
    std::string error;
    std::unique_ptr<Descriptor> parsed_desc = Parse(desc_str, keys, error, false);
    BOOST_CHECK(success == (parsed_desc != nullptr));
    if (!success) return nullptr;

    const int64_t range_start = 0, range_end = 1, next_index = 0, timestamp = 1;

    WalletDescriptor w_desc(std::move(parsed_desc), timestamp, range_start, range_end, next_index);

    LOCK(keystore.cs_wallet);

    return Assert(keystore.AddWalletDescriptor(w_desc, keys,/*label=*/"", /*internal=*/false));
};

BOOST_AUTO_TEST_CASE(ismine_standard)
{
    CKey keys[2];
    CPubKey pubkeys[2];
    for (int i = 0; i < 2; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CKey uncompressedKey = GenerateRandomKey(/*compressed=*/false);
    CPubKey uncompressedPubkey = uncompressedKey.GetPubKey();
    std::unique_ptr<interfaces::Chain>& chain = m_node.chain;

    CScript scriptPubKey;
    isminetype result;

    // P2PK compressed - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

    // P2PK compressed - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "pk(" + EncodeSecret(keys[0]) + ")";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        scriptPubKey = GetScriptForRawPubKey(pubkeys[0]);
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PK uncompressed - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

    // P2PK uncompressed - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "pk(" + EncodeSecret(uncompressedKey) + ")";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        scriptPubKey = GetScriptForRawPubKey(uncompressedPubkey);
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PKH compressed - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

    // P2PKH compressed - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "pkh(" + EncodeSecret(keys[0]) + ")";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        scriptPubKey = GetScriptForDestination(PKHash(pubkeys[0]));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2PKH uncompressed - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

    // P2PKH uncompressed - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "pkh(" + EncodeSecret(uncompressedKey) + ")";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        scriptPubKey = GetScriptForDestination(PKHash(uncompressedPubkey));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2SH - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

    // P2SH - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "sh(pkh(" + EncodeSecret(keys[0]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        CScript redeemScript = GetScriptForDestination(PKHash(pubkeys[0]));
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // (P2PKH inside) P2SH inside P2SH (invalid) - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

    // (P2PKH inside) P2SH inside P2SH (invalid) - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "sh(sh(" + EncodeSecret(keys[0]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, false);
        BOOST_CHECK_EQUAL(spk_manager, nullptr);
    }

    // (P2PKH inside) P2SH inside P2WSH (invalid) - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript redeemscript = GetScriptForDestination(PKHash(pubkeys[0]));
        CScript witnessscript = GetScriptForDestination(ScriptHash(redeemscript));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(redeemscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, witnessscript));
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // (P2PK inside) P2SH inside P2WSH (invalid) - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "wsh(sh(" + EncodeSecret(keys[0]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, false);
        BOOST_CHECK_EQUAL(spk_manager, nullptr);
    }

    // P2WPK inside P2WSH (invalid) - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript p2pk = GetScriptForRawPubKey(pubkeys[0]);
        CScript witnessscript = GetScriptForDestination(WitnessV0LongHash(0 /* version */, p2pk));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        witnessscript = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, p2pk));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript);
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, witnessscript));
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // P2WPK inside P2WSH (invalid) - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "wsh(wpk(" + EncodeSecret(keys[0]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, false);
        BOOST_CHECK_EQUAL(spk_manager, nullptr);
    }

    // (P2PK inside) P2WSH inside P2WSH (invalid) - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript witnessscript_inner = GetScriptForRawPubKey(pubkeys[0]);
        CScript witnessscript = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript_inner));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessscript));

        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript_inner));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript));
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        witnessscript = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript_inner));
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash((unsigned char)0, witnessscript));
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessscript);
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash((unsigned char)0, witnessscript));
        keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // (P2PK inside) P2WSH inside P2WSH (invalid) - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "wsh(wsh(" + EncodeSecret(keys[0]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, false);
        BOOST_CHECK_EQUAL(spk_manager, nullptr);
    }

    // P2WPK compressed - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));

        CScript witscript_inner;
        witscript_inner << ToByteVector(pubkeys[0]) << OP_CHECKSIG;

        std::vector<unsigned char> witscript;
        witscript.push_back(0x00);
        witscript.insert(witscript.end(),
                         witscript_inner.begin(),
                         witscript_inner.end());

        uint256 long_hash;
        CHash256()
            .Write(witscript)
            .Finalize(long_hash);
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        // Keystore has key and witness script
        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2WPK compressed - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "wpk(" + EncodeSecret(keys[0]) + ")";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash(/*version=*/0, pubkeys[0]));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2WPK uncompressed - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));

        CScript witscript_inner;
        witscript_inner << ToByteVector(uncompressedPubkey) << OP_CHECKSIG;

        std::vector<unsigned char> witscript;
        witscript.push_back(0x00);
        witscript.insert(witscript.end(),
                         witscript_inner.begin(),
                         witscript_inner.end());

        uint256 long_hash;
        CHash256()
            .Write(witscript)
            .Finalize(long_hash);
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        // Keystore has key, but no witness script
        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Keystore has key and witness script
        WitnessV0ScriptEntry entry(witscript);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddWitnessV0Script(entry));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // P2WPK uncompressed (invalid) - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "wpk(" + EncodeSecret(uncompressedKey) + ")";

        auto spk_manager = CreateDescriptor(keystore, desc_str, false);
        BOOST_CHECK_EQUAL(spk_manager, nullptr);
    }

    // scriptPubKey multisig - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

    // scriptPubKey multisig - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        std::string desc_str = "multi(2, " + EncodeSecret(uncompressedKey) + ", " + EncodeSecret(keys[1]) + ")";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        scriptPubKey = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2SH multisig - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

    // P2SH multisig - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());

        std::string desc_str = "sh(multi(2, " + EncodeSecret(uncompressedKey) + ", " + EncodeSecret(keys[1]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        CScript redeemScript = GetScriptForMultisig(2, {uncompressedPubkey, pubkeys[1]});
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2WSH multisig with compressed keys - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

        uint256 long_hash;
        CHash256()
            .Write(witnessScript)
            .Finalize(long_hash);
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        // Keystore has keys, but no witnessScript
        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Knowing the inner witness script is insufficient
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessScript_inner));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has keys & witnessScript
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddWitnessV0Script(entry));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        // You would be forgiven for thinking that GetScriptPubKeys() should
        // return this scriptPubKey, but it doesn't.  This is because segwit
        // scripts are handled differently from legacy CScripts for the
        // purposes that GetScriptPubKeys() is used for (mainly wallet
        // migration).
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // P2WSH multisig with compressed keys - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());

        std::string desc_str = "wsh(multi(2, " + EncodeSecret(keys[0]) + ", " + EncodeSecret(keys[1]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        CScript redeemScript = GetScriptForMultisig(2, {pubkeys[0], pubkeys[1]});
        scriptPubKey = GetScriptForDestination(WitnessV0LongHash(0 /* version */, redeemScript));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // P2WSH multisig with uncompressed key - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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

        uint256 long_hash;
        CHash256()
            .Write(witnessScript)
            .Finalize(long_hash);
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        // Keystore has keys, but no witnessScript
        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Knowing the inner witness script is insufficient
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(witnessScript_inner));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has keys & witnessScript
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddWitnessV0Script(entry));

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(long_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        scriptPubKey.clear();
        scriptPubKey << OP_0 << ToByteVector(short_hash);
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);
    }

    // P2WSH multisig with uncompressed key (invalid) - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());

        std::string desc_str = "wsh(multi(2, " + EncodeSecret(uncompressedKey) + ", " + EncodeSecret(keys[1]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, false);
        BOOST_CHECK_EQUAL(spk_manager, nullptr);
    }

    // P2WSH multisig wrapped in P2SH - Legacy
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
        keystore.SetupLegacyScriptPubKeyMan();
        LOCK(keystore.GetLegacyScriptPubKeyMan()->cs_KeyStore);

        CScript witnessScript_inner = GetScriptForMultisig(2, {pubkeys[0], pubkeys[1]});

        std::vector<unsigned char> witnessScript;
        witnessScript.push_back(0x00);
        witnessScript.insert(witnessScript.end(),
                             witnessScript_inner.begin(),
                             witnessScript_inner.end());

        uint256 long_hash;
        CHash256()
            .Write(witnessScript)
            .Finalize(long_hash);
        uint160 short_hash;
        CRIPEMD160()
            .Write(long_hash.begin(), 32)
            .Finalize(short_hash.begin());

        CScript redeemScript;
        redeemScript << OP_0 << ToByteVector(long_hash);

        // Keystore has no witnessScript, P2SH redeemScript, or keys
        scriptPubKey.clear();
        scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has witnessScript and P2SH redeemScript, but no keys
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddCScript(redeemScript));
        WitnessV0ScriptEntry entry(std::move(witnessScript));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddWitnessV0Script(entry));

        scriptPubKey.clear();
        scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 0);

        // Keystore has keys, witnessScript, P2SH redeemScript
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->AddKey(keys[1]));

        scriptPubKey.clear();
        scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
        result = keystore.GetLegacyScriptPubKeyMan()->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
        BOOST_CHECK(keystore.GetLegacyScriptPubKeyMan()->GetScriptPubKeys().count(scriptPubKey) == 1);
    }

    // P2WSH multisig - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());

        std::string desc_str = "wsh(multi(2, " + EncodeSecret(keys[0]) + ", " + EncodeSecret(keys[1]) + "))";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        CScript witnessScript = GetScriptForMultisig(2, {pubkeys[0], pubkeys[1]});
        CScript redeemScript = GetScriptForDestination(WitnessV0LongHash(0 /* version */, witnessScript));
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        result = spk_manager->IsMine(redeemScript);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);
    }

    // Combo - Descriptor
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());

        std::string desc_str = "combo(" + EncodeSecret(keys[0]) + ")";

        auto spk_manager = CreateDescriptor(keystore, desc_str, true);

        // Test P2PK
        result = spk_manager->IsMine(GetScriptForRawPubKey(pubkeys[0]));
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);

        // Test P2PKH
        result = spk_manager->IsMine(GetScriptForDestination(PKHash(pubkeys[0])));
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);

        // Test P2SH (combo descriptor does not describe P2SH)
        CScript redeemScript = GetScriptForDestination(PKHash(pubkeys[0]));
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);

        // Test P2WPK
        scriptPubKey = GetScriptForDestination(WitnessV0ShortHash(/*version=*/0, pubkeys[0]));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_SPENDABLE);

        // P2SH-P2WPK output
        redeemScript = GetScriptForDestination(WitnessV0ShortHash(/*version=*/0, pubkeys[0]));
        scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
        result = spk_manager->IsMine(scriptPubKey);
        BOOST_CHECK_EQUAL(result, ISMINE_NO);
    }

    // OP_RETURN
    {
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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
        CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
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
