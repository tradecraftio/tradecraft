// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
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

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <deploymentinfo.h>
#include <hash.h> // for signet block challenge hash
#include <script/interpreter.h>
#include <streams.h> // for parsing the genesis block
#include <util/string.h>
#include <util/system.h>

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
 * Main network on which people trade goods and services.
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
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // OriginalTargetTimespan() / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // Deployment of block-final miner commitment transaction.
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nStartTime = 1599004800; // September 2, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nTimeout = 1719878400; // July 2, 2024
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000003cda5268b682c9ebd2b");
        consensus.defaultAssumeValid = uint256S("0x000000000092ed109a133fc773421f83796aff1f6a5521256c425f39c660b60e"); // 383040

        consensus.original_adjust_interval = 2016; // two weeks
        consensus.filtered_adjust_interval = 9; // 1.5 hrs
        consensus.diff_adjust_threshold = 28336;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xd9;
        nDefaultPort = 8333;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 496;
        m_assumed_chain_state_size = 6;

        genesis = CreateGenesisBlock(1356123600, 278229610, 0x1d00ffff, 1);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000005b1e3d23ecfd2dd4a6e1a35238aa0392c0a8528c40df52376d7efe2c"));
        assert(genesis.hashMerkleRoot == uint256S("0xf53b1baa971ea40be88cf51288aabd700dfec96c486bf7155a53a4919af4c8bd"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as an addrfetch if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("seed.bitcoin.sipa.be."); // Pieter Wuille, only supports x1, x5, x9, and xd
        vSeeds.emplace_back("dnsseed.bluematt.me."); // Matt Corallo, only supports x9
        vSeeds.emplace_back("dnsseed.freicoin.dashjr.org."); // Luke Dashjr
        vSeeds.emplace_back("seed.freicoinstats.com."); // Christian Decker, supports x1 - xf
        vSeeds.emplace_back("seed.freicoin.jonasschnelli.ch."); // Jonas Schnelli, only supports x1, x5, x9, and xd
        vSeeds.emplace_back("seed.frc.petertodd.org."); // Peter Todd, only supports x1, x5, x9, and xd
        vSeeds.emplace_back("seed.freicoin.sprovoost.nl."); // Sjors Provoost
        vSeeds.emplace_back("dnsseed.emzy.de."); // Stephan Oeste
        vSeeds.emplace_back("seed.freicoin.wiz.biz."); // Jason Maurice

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "bc";

        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_main), std::end(chainparams_seed_main));

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
                {362880, uint256S("0x000000000008e9c63ddbaa03f32a6961a6837362be121b220b45410d59095f9a")},
                {372960, uint256S("0x0000000002af94c90e368a6dfd5d1f35857d3deb5a0402144866dfbab0688d09")},
                {383040, uint256S("0x000000000092ed109a133fc773421f83796aff1f6a5521256c425f39c660b60e")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
         // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 2688 000000000092ed109a133fc773421f83796aff1f6a5521256c425f39c660b60e
            .nTime    = 1689773678,
            .nTxCount = 1165936,
            .dTxRate  = 0.001581603879825156,
        };
    }
};

/**
 * Testnet: public test network which is reset from time to time.
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
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // OriginalTargetTimespan() / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // Deployment of block-final miner commitment transaction.
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nStartTime = 1599004800; // September 2, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nTimeout = 1719878400; // July 2, 2024
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000117428a7cfdf3d5299");
        consensus.defaultAssumeValid = uint256S("0x00000000000015207580bae63ac8ae344f6fdee79dbc06af9fdd88d9fe28a3e4");

        consensus.original_adjust_interval = 2016; // two weeks
        consensus.filtered_adjust_interval = 9; // 1.5 hrs
        consensus.diff_adjust_threshold = 144;

        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        nDefaultPort = 18333;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 42;
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

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet-seed.freicoin.jonasschnelli.ch.");
        vSeeds.emplace_back("seed.tfrc.petertodd.org.");
        vSeeds.emplace_back("seed.testnet.freicoin.sprovoost.nl.");
        vSeeds.emplace_back("testnet-seed.bluematt.me."); // Just a static list of stable node(s), only supports x9

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_test), std::end(chainparams_seed_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 2016, uint256S("0x00000000000017c5d079dfbe901cb7d0fae2a8eafd91be4e98f23481c73921d5")},
                {10080, uint256S("0x00000000000015207580bae63ac8ae344f6fdee79dbc06af9fdd88d9fe28a3e4")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 2688 00000000000015207580bae63ac8ae344f6fdee79dbc06af9fdd88d9fe28a3e4
            .nTime    = 1679650087,
            .nTxCount = 18550,
            .dTxRate  = 6.876553860058087e-05,
        };
    }
};

/**
 * Signet: test network with an additional consensus parameter (see BIP325).
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const ArgsManager& args) {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!args.IsArgSet("-signetchallenge")) {
            bin = ParseHex("512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae");
            vSeeds.emplace_back("seed.signet.freicoin.sprovoost.nl.");

            // Hardcoded nodes can be removed once there are more DNS seeds
            vSeeds.emplace_back("178.128.221.177");
            vSeeds.emplace_back("v7ajjeirttkbnt32wpy3c6w3emwnfr3fkla7hpxcfokr3ysd3kqtzmqd.onion:38333");

            consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000001291fc22898");
            consensus.defaultAssumeValid = uint256S("0x000000d1a0e224fa4679d2fb2187ba55431c284fa1b74cbc8cfda866fd4d2c09"); // 105495
            m_assumed_blockchain_size = 1;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 4096 000000d1a0e224fa4679d2fb2187ba55431c284fa1b74cbc8cfda866fd4d2c09
                .nTime    = 1661702566,
                .nTxCount = 1903567,
                .dTxRate  = 0.02336701143027275,
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
        consensus.LockTimeHeight = 1;
        consensus.SegwitHeight = 1;
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // OriginalTargetTimespan() / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000377ae000000000000000000000000000000000000000000000000000000");
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // Activation of block-final transactions
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].min_activation_height = 0; // No activation delay

        // message start is defined as the first 4 bytes of the sha256d of the block script
        HashWriter h{};
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 38333;
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

        bech32_hrp = "tb";

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test: intended for private networks only. Has minimal difficulty to ensure that
 * blocks can be found instantly.
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
        consensus.equilibrium_height = 0; // disable
        consensus.equilibrium_monetary_base = 0;
        consensus.initial_excess_subsidy = 0;
        consensus.truncate_inputs_activation_height = 1;
        consensus.alu_activation_height = 1;
        consensus.BIP34Height = 1; // Always active unless overridden
        consensus.BIP66Height = 1;  // Always active unless overridden
        consensus.LockTimeHeight = 1; // Always active unless overridden
        consensus.SegwitHeight = 0; // Always active unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_FINALTX].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        consensus.original_adjust_interval = 2016; // two weeks
        consensus.filtered_adjust_interval = 9; // 1.5 hrs
        consensus.diff_adjust_threshold = std::numeric_limits<int64_t>::max();

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = args.GetBoolArg("-fastprune", false) ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateGenesisBlock(1356123600, 1, 0x207fffff, 1);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x67756db06265141574ff8e7c3f97ebd57c443791e0ca27ee8b03758d6056edb8"));
        assert(genesis.hashMerkleRoot == uint256S("0xf53b1baa971ea40be88cf51288aabd700dfec96c486bf7155a53a4919af4c8bd"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();
        vSeeds.emplace_back("dummySeed.invalid.");

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("67756db06265141574ff8e7c3f97ebd57c443791e0ca27ee8b03758d6056edb8")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            {
                110,
                {AssumeutxoHash{uint256S("0x3962bcdbb1702231aa0958511d1bc261550517fa5c0bc66ed82420acc8c1c485")}, 110},
            },
            {
                200,
                {AssumeutxoHash{uint256S("0x211a567f0e90f0577256934f1607d3db6c9df986098a6183adc388d7404eb30d")}, 200},
            },
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

        bech32_hrp = "bcrt";
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

static void MaybeUpdateHeights(const ArgsManager& args, Consensus::Params& consensus)
{
    for (const std::string& arg : args.GetArgs("-testactivationheight")) {
        const auto found{arg.find('@')};
        if (found == std::string::npos) {
            throw std::runtime_error(strprintf("Invalid format (%s) for -testactivationheight=name@height.", arg));
        }
        const auto name{arg.substr(0, found)};
        const auto value{arg.substr(found + 1)};
        int32_t height;
        if (!ParseInt32(value, &height) || height < 0 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Invalid height value (%s) for -testactivationheight=name@height.", arg));
        }
        if (name == "segwit") {
            consensus.SegwitHeight = int{height};
        } else if (name == "bip34") {
            consensus.BIP34Height = int{height};
        } else if (name == "dersig") {
            consensus.BIP66Height = int{height};
        } else if (name == "locktime") {
            consensus.LockTimeHeight = int{height};
        } else {
            throw std::runtime_error(strprintf("Invalid name (%s) for -testactivationheight=name@height.", arg));
        }
    }
}

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    MaybeUpdateHeights(args, consensus);

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams = SplitString(strDeployment, ':');
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
