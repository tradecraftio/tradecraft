// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2011-2022 The Freicoin Developers
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

#include <chainparams.h>
#include <hash.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <hash.h> // for signet block challenge hash
#include <streams.h> // for parsing the genesis block
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

const std::string hexGenesisTx = "02000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d01044554656c6567726170682032372f4a756e2f3230313220426172636c61797320686974207769746820c2a33239306d2066696e65206f766572204c69626f7220666978696e67ffffffff08893428ed05000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac010000000000000023205029d180e0c5ed798d877b1ada99772986c1422ca932c41b2d0400000000000075000100000000000000fd530103202020754d31014d6574616c73207765726520616e20696d706c696369746c7920616275736976652061677265656d656e742e0a4d6f6465726e2022706170657222206973206120666c6177656420746f6f6c2c2069747320656e67696e656572696e672069732061206e657374206f66206c6565636865732e0a546865206f6c64206d6f6e6579206973206f62736f6c6574652e0a4c65742074686520696e646976696475616c206d6f6e6574697a65206974732063726564697420776974686f75742063617274656c20696e7465726d65646961726965732e0a4769766520757320612072656e742d6c657373206361736820736f2077652063616e206265206672656520666f72207468652066697273742074696d652e0a4c65742074686973206265207468652061776169746564206461776e2e7576a9140ef0f9d19a653023554146a866238b8822bc84df88ac0100000000000000fa082020202020202020754cd4224c65742075732063616c63756c6174652c20776974686f757420667572746865722061646f2c20696e206f7264657220746f207365652077686f2069732072696768742e22202d2d476f747466726965642057696c68656c6d204c6569626e697a0acebec2b4efbda5e28880efbda560efbc89e38080e38080e38080e3808020206e0aefbfa3e38080e38080e380802020efbcbce38080e380802020efbc882045efbc8920676f6f64206a6f622c206d61616b75210aefbe8ce38080e38080e3808020202fe383bd20e383bd5fefbc8fefbc8f7576a914c26be5ec809aa4bf6b30aa89823cff7cedc3679a88ac01000000000000005f06202020202020753c4963682077c3bc6e736368652046726569636f696e207669656c204572666f6c67207a756d204e75747a656e206465722039392050726f7a656e74217576a9142939acd60037281a708eb11e4e9eda452c029eca88ac0100000000000000980d20202020202020202020202020754c6d225468652076616c7565206f662061206d616e2073686f756c64206265207365656e20696e207768617420686520676976657320616e64206e6f7420696e20776861742068652069732061626c6520746f20726563656976652e22202d2d416c626572742045696e737465696e7576a914f9ca5caab4bda4dc28b5556aa79a2eec0447f0bf88ac0100000000000000800c202020202020202020202020754c5622416e2061726d79206f66207072696e6369706c65732063616e2070656e65747261746520776865726520616e2061726d79206f6620736f6c64696572732063616e6e6f742e22202d2d54686f6d6173205061696e657576a91408f320cbb41a1ae25b794f6175f96080681989f388accc60948c0b0000001976a91485e54144c4020a65fa0a8fdbac8bba75dbc2fd0088ac0000000000000000";

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion)
{
    CMutableTransaction txNew;
    CDataStream stream(ParseHex(hexGenesisTx), SER_NETWORK, PROTOCOL_VERSION);
    stream >> txNew;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.bitcoin_mode = false;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 0; // never
        consensus.perpetual_subsidy = 9536743164; // 95.367,431,64fc
        consensus.equilibrium_height = 161280; // three years
        consensus.equilibrium_monetary_base = 10000000000000000LL; // 100,000,000.0000,0000fc
        consensus.initial_excess_subsidy = 15916928404LL; // 1519.1692,8404fc
        consensus.truncate_inputs_activation_height = 158425;
        consensus.alu_activation_height = 229376;
        consensus.BIP34Height = 1;
        consensus.BIP66Height = 158425; // 0000000000000799b28bbc61b9a93770af898ffc621174e70480656f0382a020
        consensus.LockTimeHeight = 258048; // 000000000000002b7c1e4b345d09ed56475bd7e9d84f1bb43ea13195aa7719b6
        consensus.SegwitHeight = 278208; // 0000000000000050599fa4cae6de65d71a1d7d0d7dc2e9b19531b794c30458c0
        consensus.MinBIP9WarningHeight = 280224; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.aux_pow_limit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.aux_pow_target_spacing = 15 * 60;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // OriginalTargetTimespan() / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 27;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // Deployment of block-final miner commitment transaction.
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nStartTime = 1562068800; // July 2, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nTimeout = 1587038400; // April 16, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].min_activation_height = 0; // No activation delay

        // Deployment of merge mining
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].nStartTime = 1591056000; // June 2nd, 2020.
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].nTimeout = 1622592000; // June 2nd, 2021.
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000003a522d645e750b6c9b5");
        consensus.defaultAssumeValid = uint256S("0x00000000038a44f3a253d12f27dcc9330967748fd17ce807e61598fc22cf9d6f"); // 352800

        // Locked-in via checkpoint:
        consensus.verify_coinbase_lock_time_activation_height = 247554;
        // Wednesday, October 2, 2019 00:00:00 UTC
        // This is 4PM PDT, 7PM EDT, and 9AM JST.
        consensus.verify_coinbase_lock_time_timeout = 1569974400;

        /**
         * The protocol cleanup rule change is scheduled for activation on 16
         * Apr 2021 at midnight UTC.  This is 4PM PDT, 7PM EDT, and 9AM JST.
         * Since the activation time is median-time-past, it'll actually trigger
         * about 90 minutes after this wall-clock time.  Note that the auxpow
         * soft-fork must activate before the protocol cleanup rule change.
         */
        consensus.protocol_cleanup_activation_time = 1618531200;

        /**
         * The size expansion rule change is scheduled for activation on 16 Oct
         * 2024 at midnight UTC.  This is 4PM PDT, 7PM EDT, and 9AM JST.  Since
         * the activation time is median-time-past, it'll actually trigger about
         * 90 minutes after this wall-clock time.
         *
         * This date is chosen to be roughly 2 years after the expected release
         * date of official binaries.  While the Freicoin developer team doesn't
         * have the resources to provide strong ongoing support beyond emergency
         * fixes, we nevertheless have an ideal goal of supporting release
         * binaries for up to 2 years following the first release from that
         * series.  Any release of a new series prior to the deployment of
         * forward blocks should set this to be at least two years from the time
         * of release.  When forward blocks is deployed, this parameter should
         * be set to the highest value used in prior releases, and becomes the
         * earliest time at which the hard-fork rules can activate.
         */
        consensus.size_expansion_activation_time = 1729062000;

        consensus.original_adjust_interval = 2016; // two weeks
        consensus.filtered_adjust_interval = 9; // 1.5 hrs
        consensus.diff_adjust_threshold = 28336;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x2c;
        pchMessageStart[1] = 0xfe;
        pchMessageStart[2] = 0x7e;
        pchMessageStart[3] = 0x6d;
        nDefaultPort = 8639;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 350;
        m_assumed_chain_state_size = 6;

        genesis = CreateGenesisBlock(1356123600, 278229610, 0x1d00ffff, 1);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000005b1e3d23ecfd2dd4a6e1a35238aa0392c0a8528c40df52376d7efe2c"));
        assert(genesis.hashMerkleRoot == uint256S("0xf53b1baa971ea40be88cf51288aabd700dfec96c486bf7155a53a4919af4c8bd"));

        uint256 tag;
        CSHA256().Write(reinterpret_cast<const unsigned char*>("auxpow"), 6).Finalize(tag.begin());
        CHashWriter hw(SER_GETHASH, PROTOCOL_VERSION);
        hw << tag;
        hw << tag;
        hw << genesis;
        consensus.aux_pow_path = hw.GetHash();
        assert(consensus.aux_pow_path == uint256S("0x632938ec752e63b7f63cdd9a16b336c6c5cefbaad66278e402ce59d706f57ff6"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as an addrfetch if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("seed.freico.in"); // Mark Friedenbach
        vSeeds.emplace_back("fledge.freico.in"); // @galambo
        vSeeds.emplace_back("dnsseed.sicanet.net"); // Fredrik Bodin
        vSeeds.emplace_back("ap-northeast-1.aws.seed.tradecraft.io");
        vSeeds.emplace_back("eu-west-1.aws.seed.tradecraft.io");
        vSeeds.emplace_back("us-west-2.aws.seed.tradecraft.io");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "fc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 10080, uint256S("0x00000000003ff9c4b806639ec4376cc9acafcdded0e18e9dbcc2fc42e8e72331")},
                { 20160, uint256S("0x0000000000009708ba48a52599295db8b9ec5d29148561e6ac850af765026528")},
                { 28336, uint256S("0x000000000000cc374a984c0deec9aed6fff764918e2cfd4be6670dd4d5292ccb")}, // Difficulty adjustment hard-fork activation
                { 30240, uint256S("0x0000000000004acbe1ed430d4a70d8a9ac62daa849e0bc708da52eeba8f39afc")},
                { 40320, uint256S("0x0000000000008ad31a52a3e749bd5c477aa3da18cc0acd3e3d944b5edc58e7bd")},
                { 50400, uint256S("0x0000000000000e2e3686f1eb852222ffff33a403947478bea143ed88c81fdd87")},
                { 60480, uint256S("0x000000000000029a0d1df882d1ddd15387855d5f904127c25359f8bdc6425928")},
                { 70560, uint256S("0x00000000000002b41cead9ce01c519a56998db8a715aae518f4b72403d6dc95a")},
                { 80640, uint256S("0x00000000000001c20353e3df80d35c8348bc07940d5e08d4740372ef45a4474f")},
                { 90720, uint256S("0x00000000000006c884dfe4e81504fd8eaf9d7d770a04dbdafb2cbf5ad7ab64c6")},
                {100800, uint256S("0x00000000000004dc5badc155de4d07b4c09b9f3ecfdfdaf71576f3d2be192ea3")},
                {110880, uint256S("0x0000000000000ef59288c01fcef9c26b0457bc93ca106d06bb10cd5dfad7fca9")},
                {120960, uint256S("0x00000000000002968c68497ec2a7ec6b5030202dbf874126a65e437f53c03bea")},
                {131040, uint256S("0x0000000000000bf11095c39e143ed02508132e48e040db791a0e7ed73378e7ed")},
                {141120, uint256S("0x000000000000016331fe98568de3673c7c983f10d4ceab0f75d928acc0378001")},
                {151200, uint256S("0x000000000000047df778aaa84d03cf2d8f9b51ef530a7d3708bfd6a9e0dd5d41")},
                {161280, uint256S("0x00000000000021b3611f18840adf738c4a0c8de1479f53721c29a899620a4064")},
                {171360, uint256S("0x00000000000037920bd0a1f13c579ca7c6ade2ef56b19027dd4408c292e5882f")},
                {181440, uint256S("0x00000000000001d49e7ad75303c6217d6205cd51d5c1cc494427418385976d44")},
                {191520, uint256S("0x000000000000034be18ec2f1ca59bbc70d54a9cb10fc7230122297c037f441ee")},
                {201600, uint256S("0x00000000000004bb0cc14b70f9fd72900a6839731892d959764dd89615a5535a")},
                {211680, uint256S("0x00000000000000e1156dafc83bc94c1508fbaa2ec1b1440aeceac7dfc0944664")},
                {221760, uint256S("0x00000000000000a7ca764843bedea1e8c7eb2e22390aca9d133caafcd0842ea1")},
                {231840, uint256S("0x000000000000000d1e7c399c42e260076f541b1d41bb805af46994ce896befe7")},
                {241920, uint256S("0x000000000000007f4809ec08659c88598624743896e8620d4a7ebb36ede698f9")},
                {252000, uint256S("0x00000000000000437687524302491d9aead11eb0090a5c451a4dbe6f85d4fbe1")},
                {262080, uint256S("0x000000000000001332e59516a8156b56de7f7ca804238402732f7de4470da1a0")},
                {272160, uint256S("0x000000000000002781d74d59a2e0edaf3b14b5435d8de67c1ed7b547e5f67752")},
                {282240, uint256S("0x00000000000000b852854b82afcff8caf86fc2f392b9e4a4814bf47977813fc1")},
                {292320, uint256S("0x000000000000140206e6fe913172634efa63c3928b0305052bfe4078f1a636fd")},
                {302400, uint256S("0x000000000000114100284febd7d76aadf7522062dabf611c73f4f9b44db72c35")},
                {312480, uint256S("0x0000000000000bc166f4cd03a22952161cbd1b79eff595c17b6904d21307d17a")},
                {322560, uint256S("0x0000000000000c6e3b938bc8dddf6c05a8ce4b4d46af273d4af4bea53c23ea84")},
                {332640, uint256S("0x0000000000000f985237422cd4dc7262ab7a18cd8294b2f721d408caaafe7075")},
                {342720, uint256S("0x00000000000006de444cdd02145c4eaa0960083997afae98a03b32d84796ea63")},
                {352800, uint256S("0x00000000038a44f3a253d12f27dcc9330967748fd17ce807e61598fc22cf9d6f")},
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 2688 00000000038a44f3a253d12f27dcc9330967748fd17ce807e61598fc22cf9d6f
            /* nTime    */ 1660340295,
            /* nTxCount */ 1105361,
            /* dTxRate  */ 0.002238592473079549,
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.bitcoin_mode = false;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 0; // never
        consensus.perpetual_subsidy = 9536743164; // 95.367,431,64fc
        consensus.equilibrium_height = 0; // disable
        consensus.equilibrium_monetary_base = 0;
        consensus.initial_excess_subsidy = 0;
        consensus.truncate_inputs_activation_height = 1;
        consensus.alu_activation_height = 1;
        consensus.BIP34Height = std::numeric_limits<int>::max(); // BIP34 never activated on Freicoin's testnet
        consensus.BIP66Height = 1; // 0000000000002076358270b88c18cce6a0886c870e6167776e40d167bd01b49f
        consensus.LockTimeHeight = 1512; // 00000000000019f427d3b84e5d97485fa957deb7c5d7df6ca7a60f5739b91d3a
        consensus.SegwitHeight = 2016; // 00000000000017c5d079dfbe901cb7d0fae2a8eafd91be4e98f23481c73921d5
        consensus.MinBIP9WarningHeight = 4032; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.aux_pow_limit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.aux_pow_target_spacing = 15 * 60;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 378; // 75% for testchains
        consensus.nMinerConfirmationWindow = 504;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 27;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // Deployment of block-final miner commitment transaction.
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nStartTime = 1562068800; // July 2, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nTimeout = 1622592000; // June 2nd, 2021.
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].min_activation_height = 0; // No activation delay

        // Deployment of merge mining
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].nStartTime = 1591056000; // June 2nd, 2020.
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].nTimeout = 1622592000; // June 2nd, 2021.
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000b5f8d7a875bd74");
        consensus.defaultAssumeValid = uint256S("0x00000000000017c5d079dfbe901cb7d0fae2a8eafd91be4e98f23481c73921d5"); // 2016

        consensus.verify_coinbase_lock_time_activation_height = 2016;
        // Tuesday, April 2, 2019 00:00:00 UTC
        consensus.verify_coinbase_lock_time_timeout = 1554163200;

        // Two months prior to main net
        // 16 November 2020 00:00:00 UTC
        consensus.protocol_cleanup_activation_time = 1605484800;

        // Nine months prior to main net
        // 16 January 2024 00:00:00 UTC
        consensus.size_expansion_activation_time = 1705392000;

        consensus.original_adjust_interval = 2016; // two weeks
        consensus.filtered_adjust_interval = 9; // 1.5 hrs
        consensus.diff_adjust_threshold = 144;

        pchMessageStart[0] = 0x5e;
        pchMessageStart[1] = 0xd6;
        pchMessageStart[2] = 0x7c;
        pchMessageStart[3] = 0xf3;
        nDefaultPort = 18639;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 40;
        m_assumed_chain_state_size = 2;

        const std::string timestamp("The Times 7/Aug/2020 Foreign Office cat quits to spend more time with family");
        CMutableTransaction genesis_tx;
        genesis_tx.nVersion = 2;
        genesis_tx.vin.resize(1);
        genesis_tx.vin[0].prevout.SetNull();
        genesis_tx.vin[0].scriptSig = CScript() << 0 << std::vector<unsigned char>((const unsigned char*)timestamp.data(), (const unsigned char*)timestamp.data() + timestamp.length());
        genesis_tx.vin[0].nSequence = 0xffffffff;
        genesis_tx.vout.resize(1);
        genesis_tx.vout[0].SetReferenceValue(consensus.perpetual_subsidy);
        genesis_tx.vout[0].scriptPubKey = CScript() << OP_RETURN;
        genesis_tx.nLockTime = 1596931200;
        genesis_tx.lock_height = 0;

        genesis = CBlock();
        genesis.nVersion = 1;
        genesis.hashPrevBlock.SetNull();
        genesis.nTime = 1596931200;
        genesis.nBits = 0x1d00ffff;
        genesis.nNonce = 1566443406UL;
        genesis.vtx.push_back(MakeTransactionRef(std::move(genesis_tx)));
        genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000003b5183593282fd30d3d7e79243eb883d6c2d8670f69811c6b9a76585"));
        assert(genesis.hashMerkleRoot == uint256S("0xda41f94f1a4a7d4a5cd54245bf4ad423da65a292a4de6d86d7746c4ad41e7ee7"));

        uint256 tag;
        CSHA256().Write(reinterpret_cast<const unsigned char*>("auxpow"), 6).Finalize(tag.begin());
        CHashWriter hw(SER_GETHASH, PROTOCOL_VERSION);
        hw << tag;
        hw << tag;
        hw << genesis;
        consensus.aux_pow_path = hw.GetHash();
        assert(consensus.aux_pow_path == uint256S("0xe99fc44bfacee2f7e28d135845ff8a385d6d31353928d5b499700f1a2ad1b18b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tf";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {2016, uint256S("0x00000000000017c5d079dfbe901cb7d0fae2a8eafd91be4e98f23481c73921d5")},
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 2016 00000000000017c5d079dfbe901cb7d0fae2a8eafd91be4e98f23481c73921d5
            /* nTime    */ 1596962899,
            /* nTxCount */ 2422,
            /* dTxRate  */ 0.002253125,
        };
    }
};

/**
 * Signet
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const ArgsManager& args) {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!args.IsArgSet("-signetchallenge")) {
            bin = ParseHex("512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae");
            vSeeds.emplace_back("178.128.221.177");
            vSeeds.emplace_back("2a01:7c8:d005:390::5");
            vSeeds.emplace_back("v7ajjeirttkbnt32wpy3c6w3emwnfr3fkla7hpxcfokr3ysd3kqtzmqd.onion:38333");

            consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000000019fd16269a");
            consensus.defaultAssumeValid = uint256S("0x0000002a1de0f46379358c1fd09906f7ac59adf3712323ed90eb59e4c183c020"); // 9434
            m_assumed_blockchain_size = 1;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 4096 0000002a1de0f46379358c1fd09906f7ac59adf3712323ed90eb59e4c183c020
                /* nTime    */ 1603986000,
                /* nTxCount */ 9582,
                /* dTxRate  */ 0.00159272030651341,
            };
        } else {
            const auto signet_challenge = args.GetArgs("-signetchallenge");
            if (signet_challenge.size() != 1) {
                throw std::runtime_error(strprintf("%s: -signetchallenge cannot be multiple values.", __func__));
            }
            bin = ParseHex(signet_challenge[0]);

            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", signet_challenge[0]);
        }

        if (args.IsArgSet("-signetseednode")) {
            vSeeds = args.GetArgs("-signetseednode");
        }

        consensus.verify_coinbase_lock_time_activation_height = std::numeric_limits<int64_t>::max();
        consensus.verify_coinbase_lock_time_timeout = 1356123600;

        // Two months prior to main net
        // 16 November 2020 00:00:00 UTC
        consensus.protocol_cleanup_activation_time = 1605484800;

        // Nine months prior to main net
        // 16 January 2024 00:00:00 UTC
        consensus.size_expansion_activation_time = 1705392000;

        consensus.original_adjust_interval = 2016; // two weeks
        consensus.filtered_adjust_interval = 9; // 1.5 hrs
        consensus.diff_adjust_threshold = std::numeric_limits<int64_t>::max();

        strNetworkID = CBaseChainParams::SIGNET;
        consensus.bitcoin_mode = false;
        consensus.signet_blocks = true;
        consensus.signet_challenge.assign(bin.begin(), bin.end());
        consensus.nSubsidyHalvingInterval = 0; // never
        consensus.perpetual_subsidy = 9536743164; // 95.367,431,64fc
        consensus.equilibrium_height = 161280; // three years
        consensus.equilibrium_monetary_base = 10000000000000000LL; // 100,000,000.0000,0000fc
        consensus.initial_excess_subsidy = 15916928404LL; // 1519.1692,8404fc
        consensus.BIP34Height = 1;
        consensus.BIP66Height = 1;
        consensus.SegwitHeight = 1;
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // OriginalTargetTimespan() / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000377ae000000000000000000000000000000000000000000000000000000");
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 27;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // Activation of block-final transaction
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nStartTime = 1599004800; // September 2, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nTimeout = 1719878400; // July 2, 2024
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].min_activation_height = 0; // No activation delay

        // Deployment of merge mining
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].nStartTime = 1662145200; // September 2, 2022
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].nTimeout = 1783018800; // July 2, 2026
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].min_activation_height = 0; // No activation delay


        // message start is defined as the first 4 bytes of the sha256d of the block script
        CHashWriter h(SER_DISK, 0);
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 38639;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1598918400, 5293684, 0x1e0377ae, 1);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000500fc45aa5ed5763371527daca0ddc04212352e4759b8c9b563cc53934"));
        assert(genesis.hashMerkleRoot == uint256S("0xf53b1baa971ea40be88cf51288aabd700dfec96c486bf7155a53a4919af4c8bd"));

        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tf";

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.bitcoin_mode = false;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 150;
        consensus.perpetual_subsidy = 5000000000; // 50.000,000,00fc
        consensus.equilibrium_height = 1; // disable
        consensus.equilibrium_monetary_base = 0;
        consensus.initial_excess_subsidy = 0;
        consensus.truncate_inputs_activation_height = 1;
        consensus.alu_activation_height = 1;
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.LockTimeHeight = 432; // locktime activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0; // SEGWIT is always activated on regtest unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.aux_pow_limit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.aux_pow_target_spacing = 15 * 60;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 27;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_AUXPOW].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        consensus.verify_coinbase_lock_time_activation_height = std::numeric_limits<int64_t>::max();
        consensus.verify_coinbase_lock_time_timeout = 1356123600;

        /**
         * Effectively never.
         *
         * Unit tests which check the protocol cleanup rule activation should
         * set this consensus parameter manually for the duration of the
         * test. Setting it to a real value here would make other unit tests
         * checking pre-activationn rules fail at some point in the future,
         * which is unacceptable time-dependency in the build process.
         */
        consensus.protocol_cleanup_activation_time = std::numeric_limits<int64_t>::max();
        consensus.size_expansion_activation_time = std::numeric_limits<int64_t>::max();

        consensus.original_adjust_interval = 2016; // two weeks
        consensus.filtered_adjust_interval = 9; // 1.5 hrs
        consensus.diff_adjust_threshold = std::numeric_limits<int64_t>::max();

        pchMessageStart[0] = 0xed;
        pchMessageStart[1] = 0x99;
        pchMessageStart[2] = 0x9c;
        pchMessageStart[3] = 0xf6;
        nDefaultPort = 28639;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateGenesisBlock(1356123600, 1, 0x207fffff, 1);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x67756db06265141574ff8e7c3f97ebd57c443791e0ca27ee8b03758d6056edb8"));
        assert(genesis.hashMerkleRoot == uint256S("0xf53b1baa971ea40be88cf51288aabd700dfec96c486bf7155a53a4919af4c8bd"));

        uint256 tag;
        CSHA256().Write(reinterpret_cast<const unsigned char*>("auxpow"), 6).Finalize(tag.begin());
        CHashWriter hw(SER_GETHASH, PROTOCOL_VERSION);
        hw << tag;
        hw << tag;
        hw << genesis;
        consensus.aux_pow_path = hw.GetHash();
        assert(consensus.aux_pow_path == uint256S("0xd799d41af01c1ac77e6a7793ba046a7432bb6ec250b84e2f5c6f225e05f0fc74"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("0x67756db06265141574ff8e7c3f97ebd57c443791e0ca27ee8b03758d6056edb8")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "fcrt";
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int min_activation_height)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        consensus.vDeployments[d].min_activation_height = min_activation_height;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (args.IsArgSet("-segwitheight")) {
        int64_t height = args.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() < 3 || 4 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
        }
        int64_t nStartTime, nTimeout;
        int min_activation_height = 0;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 4 && !ParseInt32(vDeploymentParams[3], &min_activation_height)) {
            throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, min_activation_height);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%d\n", vDeploymentParams[0], nStartTime, nTimeout, min_activation_height);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::SIGNET) {
        return std::unique_ptr<CChainParams>(new SigNetParams(args));
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}
