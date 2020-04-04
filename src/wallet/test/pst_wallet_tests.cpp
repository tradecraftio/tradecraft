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

#include <key_io.h>
#include <script/sign.h>
#include <utilstrencodings.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <univalue.h>

#include <boost/test/unit_test.hpp>
#include <test/test_freicoin.h>
#include <wallet/test/wallet_test_fixture.h>

extern bool ParseHDKeypath(std::string keypath_str, std::vector<uint32_t>& keypath);

BOOST_FIXTURE_TEST_SUITE(pst_wallet_tests, WalletTestingSetup)

BOOST_AUTO_TEST_CASE(pst_updater_test)
{
    // Create prevtxs and add to wallet
    CDataStream s_prev_tx1(ParseHex("02000000ff0101287879a23b890aa1c4748be1e50d5149314b03de7963795ddd245c6cc9c9d2ae010000001716001440e65a1237e05ede683dd8bf46f3068570df3579feffffff02d8231f1b0100000017a914aed962d6654f9a2b36608eb9d64d2b260db4f1118700c2eb0b00000000160014c2a21cf005338119380d70e7e3b5cb738aa3280f03483045022100a22edcc6e5bc511af4cc4ae0de0fcd75c7e04d8c1c3a8aa9d820ed4b967384ec02200642963597b9b1bc22c75e9f3e117284a962188bf5e8a74c895089046a20ad7701240021035509a48eb623e10aace8bfd0212fdb8a8e5af3c94b0b133b95e114cab89e4f79ac006500000001000000"), SER_NETWORK, PROTOCOL_VERSION);
    CTransactionRef prev_tx1;
    s_prev_tx1 >> prev_tx1;
    CWalletTx prev_wtx1(&m_wallet, prev_tx1);
    m_wallet.mapWallet.emplace(prev_wtx1.GetHash(), std::move(prev_wtx1));

    CDataStream s_prev_tx2(ParseHex("0200000001aad73931018bd25f84ae400b68848be09db706eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c015bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82c833af241c78c17ddbde493487d0f20a270100000017a914eec88a701c971c477f6e954856bca3581805e182876500000001000000"), SER_NETWORK, PROTOCOL_VERSION);
    CTransactionRef prev_tx2;
    s_prev_tx2 >> prev_tx2;
    CWalletTx prev_wtx2(&m_wallet, prev_tx2);
    m_wallet.mapWallet.emplace(prev_wtx2.GetHash(), std::move(prev_wtx2));

    // Add scripts
    CScript rs1;
    CDataStream s_rs1(ParseHex("475221029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d752ae"), SER_NETWORK, PROTOCOL_VERSION);
    s_rs1 >> rs1;
    m_wallet.AddCScript(rs1);

    CScript rs2;
    CDataStream s_rs2(ParseHex("2200203b534516e1279d47a36c3216b440a721c2b7d081d9d48bcd6972007a25623e88"), SER_NETWORK, PROTOCOL_VERSION);
    s_rs2 >> rs2;
    m_wallet.AddCScript(rs2);

    CScript ws1;
    CDataStream s_ws1(ParseHex("47522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc21023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae"), SER_NETWORK, PROTOCOL_VERSION);
    s_ws1 >> ws1;
    m_wallet.AddCScript(ws1);
    std::vector<unsigned char> witscript{0x00};
    witscript.insert(witscript.end(), ws1.begin(), ws1.end());
    m_wallet.AddWitnessV0Script(witscript);

    // Add hd seed
    CKey key = DecodeSecret("5KSSJQ7UNfFGwVgpCZDSHm5rVNhMFcFtvWM3zQ8mW4qNDEN7LFd"); // Mainnet and uncompressed form of cUkG8i1RFfWGWy5ziR11zJ5V4U4W3viSFCfyJmZnvQaUsd1xuF3T
    CPubKey master_pub_key = m_wallet.DeriveNewSeed(key);
    m_wallet.SetHDSeed(master_pub_key);
    m_wallet.NewKeyPool();

    // Call FillPST
    PartiallySignedTransaction pstx;
    CDataStream ssData(ParseHex("707374ff01009e0200000002287879a23b890aa1c4748be1e50d5149314b03de7963795ddd245c6cc9c9d2ae0000000000ffffffffc687198220ae3421153e059c72fea5807512be182187199ec0e81d0638cc44e20100000000ffffffff0270aaf0080000000016001488827283b979b11be43a90c62abcb31b7a371aaa00e1f50500000000160014ebd77365e9ee337be7a271b5a48ad5daae91f85a00000000010000000000000000"), SER_NETWORK, PROTOCOL_VERSION);
    ssData >> pstx;

    // Fill transaction with our data
    FillPST(&m_wallet, pstx, SIGHASH_ALL, false, true);

    // Get the final tx
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << pstx;
    std::string final_hex = HexStr(ssTx.begin(), ssTx.end());
    BOOST_CHECK_EQUAL(final_hex, "707374ff01009e0200000002287879a23b890aa1c4748be1e50d5149314b03de7963795ddd245c6cc9c9d2ae0000000000ffffffffc687198220ae3421153e059c72fea5807512be182187199ec0e81d0638cc44e20100000000ffffffff0270aaf0080000000016001488827283b979b11be43a90c62abcb31b7a371aaa00e1f50500000000160014ebd77365e9ee337be7a271b5a48ad5daae91f85a0000000001000000000100bf0200000001aad73931018bd25f84ae400b68848be09db706eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c015bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82c833af241c78c17ddbde493487d0f20a270100000017a914eec88a701c971c477f6e954856bca3581805e1828765000000010000000104475221029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d752ae2206029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c2183f1ab96e07f10d90c6a4f000000800000008000000080220602dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d710d90c6a4f0000008000000080010000800001012300c2eb0b00000000160014c2a21cf005338119380d70e7e3b5cb738aa3280f0100000001054800522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc21023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae00002206023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7310d90c6a4f000000800000008003000080220603089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc10d90c6a4f00000080000000800200008000010124002103a9a4c37f5996d3aa25dbac6b570af0650394492942460b354753ed9eeca58771ac0000220203a9a4c37f5996d3aa25dbac6b570af0650394492942460b354753ed9eeca5877110d90c6a4f000000800000008004000080000101240021027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b34282a94b6b50051096ac00002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b34282a94b6b5005109610d90c6a4f00000080000000800500008000");
}

BOOST_AUTO_TEST_CASE(parse_hd_keypath)
{
    std::vector<uint32_t> keypath;

    BOOST_CHECK(ParseHDKeypath("1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1", keypath));
    BOOST_CHECK(!ParseHDKeypath("///////////////////////////", keypath));

    BOOST_CHECK(ParseHDKeypath("1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1'/1", keypath));
    BOOST_CHECK(!ParseHDKeypath("//////////////////////////'/", keypath));

    BOOST_CHECK(ParseHDKeypath("1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/", keypath));
    BOOST_CHECK(!ParseHDKeypath("1///////////////////////////", keypath));

    BOOST_CHECK(ParseHDKeypath("1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1'/", keypath));
    BOOST_CHECK(!ParseHDKeypath("1/'//////////////////////////", keypath));

    BOOST_CHECK(ParseHDKeypath("", keypath));
    BOOST_CHECK(!ParseHDKeypath(" ", keypath));

    BOOST_CHECK(ParseHDKeypath("0", keypath));
    BOOST_CHECK(!ParseHDKeypath("O", keypath));

    BOOST_CHECK(ParseHDKeypath("0000'/0000'/0000'", keypath));
    BOOST_CHECK(!ParseHDKeypath("0000,/0000,/0000,", keypath));

    BOOST_CHECK(ParseHDKeypath("01234", keypath));
    BOOST_CHECK(!ParseHDKeypath("0x1234", keypath));

    BOOST_CHECK(ParseHDKeypath("1", keypath));
    BOOST_CHECK(!ParseHDKeypath(" 1", keypath));

    BOOST_CHECK(ParseHDKeypath("42", keypath));
    BOOST_CHECK(!ParseHDKeypath("m42", keypath));

    BOOST_CHECK(ParseHDKeypath("4294967295", keypath)); // 4294967295 == 0xFFFFFFFF (uint32_t max)
    BOOST_CHECK(!ParseHDKeypath("4294967296", keypath)); // 4294967296 == 0xFFFFFFFF (uint32_t max) + 1

    BOOST_CHECK(ParseHDKeypath("m", keypath));
    BOOST_CHECK(!ParseHDKeypath("n", keypath));

    BOOST_CHECK(ParseHDKeypath("m/", keypath));
    BOOST_CHECK(!ParseHDKeypath("n/", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0", keypath));
    BOOST_CHECK(!ParseHDKeypath("n/0", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0'", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/0''", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0'/0'", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/'0/0'", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/0", keypath));
    BOOST_CHECK(!ParseHDKeypath("n/0/0", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/0/00", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/0/0/f00", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/0/000000000000000000000000000000000000000000000000000000000000000000000000000000000000", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/1/1/111111111111111111111111111111111111111111111111111111111111111111111111111111111111", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/00/0", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/0'/00/'0", keypath));

    BOOST_CHECK(ParseHDKeypath("m/1/", keypath));
    BOOST_CHECK(!ParseHDKeypath("m/1//", keypath));

    BOOST_CHECK(ParseHDKeypath("m/0/4294967295", keypath)); // 4294967295 == 0xFFFFFFFF (uint32_t max)
    BOOST_CHECK(!ParseHDKeypath("m/0/4294967296", keypath)); // 4294967296 == 0xFFFFFFFF (uint32_t max) + 1

    BOOST_CHECK(ParseHDKeypath("m/4294967295", keypath)); // 4294967295 == 0xFFFFFFFF (uint32_t max)
    BOOST_CHECK(!ParseHDKeypath("m/4294967296", keypath)); // 4294967296 == 0xFFFFFFFF (uint32_t max) + 1
}

BOOST_AUTO_TEST_SUITE_END()
