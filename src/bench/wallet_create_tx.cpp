// Copyright (c) 2022 The Bitcoin Core developers
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

#include <bench/bench.h>
#include <chainparams.h>
#include <wallet/coincontrol.h>
#include <consensus/merkle.h>
#include <kernel/chain.h>
#include <node/context.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

using wallet::CWallet;
using wallet::CreateMockableWalletDatabase;
using wallet::WALLET_FLAG_DESCRIPTORS;

struct TipBlock
{
    uint256 prev_block_hash;
    int64_t prev_block_time;
    int tip_height;
};

TipBlock getTip(const CChainParams& params, const node::NodeContext& context)
{
    auto tip = WITH_LOCK(::cs_main, return context.chainman->ActiveTip());
    return (tip) ? TipBlock{tip->GetBlockHash(), tip->GetBlockTime(), tip->nHeight} :
           TipBlock{params.GenesisBlock().GetHash(), params.GenesisBlock().GetBlockTime(), 0};
}

void generateFakeBlock(const CChainParams& params,
                       const node::NodeContext& context,
                       CWallet& wallet,
                       const CScript& coinbase_out_script)
{
    TipBlock tip{getTip(params, context)};

    // Create block
    CBlock block;
    CMutableTransaction coinbase_tx;
    coinbase_tx.lock_height = tip.tip_height;
    coinbase_tx.vin.resize(1);
    coinbase_tx.vin[0].prevout.SetNull();
    coinbase_tx.vout.resize(2);
    coinbase_tx.vout[0].scriptPubKey = coinbase_out_script;
    coinbase_tx.vout[0].SetReferenceValue(49 * COIN);
    coinbase_tx.vin[0].scriptSig = CScript() << ++tip.tip_height << OP_0;
    coinbase_tx.vout[1].scriptPubKey = coinbase_out_script; // extra output
    coinbase_tx.vout[1].SetReferenceValue(1 * COIN);
    block.vtx = {MakeTransactionRef(std::move(coinbase_tx))};

    block.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
    block.hashPrevBlock = tip.prev_block_hash;
    block.hashMerkleRoot = BlockMerkleRoot(block);
    block.nTime = ++tip.prev_block_time;
    block.nBits = params.GenesisBlock().nBits;
    block.nNonce = 0;

    {
        LOCK(::cs_main);
        // Add it to the index
        CBlockIndex* pindex{context.chainman->m_blockman.AddToBlockIndex(block, context.chainman->m_best_header)};
        // add it to the chain
        context.chainman->ActiveChain().SetTip(*pindex);
    }

    // notify wallet
    const auto& pindex = WITH_LOCK(::cs_main, return context.chainman->ActiveChain().Tip());
    wallet.blockConnected(ChainstateRole::NORMAL, kernel::MakeBlockInfo(pindex, &block));
}

struct PreSelectInputs {
    // How many coins from the wallet the process should select
    int num_of_internal_inputs;
    // future: this could have external inputs as well.
};

static void WalletCreateTx(benchmark::Bench& bench, const OutputType output_type, bool allow_other_inputs, std::optional<PreSelectInputs> preset_inputs)
{
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    // Set clock to genesis block, so the descriptors/keys creation time don't interfere with the blocks scanning process.
    SetMockTime(test_setup->m_node.chainman->GetParams().GenesisBlock().nTime);
    CWallet wallet{test_setup->m_node.chain.get(), "", CreateMockableWalletDatabase()};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetupDescriptorScriptPubKeyMans();
    }

    // Generate destinations
    const auto dest{getNewDestination(wallet, output_type)};

    // Generate chain; each coinbase will have two outputs to fill-up the wallet
    const auto& params = Params();
    const CScript coinbase_out{GetScriptForDestination(dest)};
    unsigned int chain_size = 5000; // 5k blocks means 10k UTXO for the wallet (minus 200 due COINBASE_MATURITY)
    for (unsigned int i = 0; i < chain_size; ++i) {
        generateFakeBlock(params, test_setup->m_node, wallet, coinbase_out);
    }

    // Check available balance
    int next_height = test_setup->m_node.chain->getHeight().value() + 1;
    auto bal = WITH_LOCK(wallet.cs_wallet, return wallet::AvailableCoins(wallet, next_height).GetTotalAmount()); // Cache
    CAmount total{0};
    for (unsigned int height = 0; height < chain_size - COINBASE_MATURITY; ++height) {
        total += GetTimeAdjustedValue(49 * COIN, next_height - height);
        total += GetTimeAdjustedValue( 1 * COIN, next_height - height);
    }
    assert(bal == total);

    wallet::CCoinControl coin_control;
    coin_control.m_allow_other_inputs = allow_other_inputs;

    CAmount target = 0;
    if (preset_inputs) {
        // Select inputs, each has 49 FRC
        wallet::CoinFilterParams filter_coins;
        filter_coins.max_count = preset_inputs->num_of_internal_inputs;
        const auto& res = WITH_LOCK(wallet.cs_wallet,
                                    return wallet::AvailableCoins(wallet, next_height, /*coinControl=*/nullptr, /*feerate=*/std::nullopt, filter_coins));
        for (int i=0; i < preset_inputs->num_of_internal_inputs; i++) {
            const auto& coin{res.coins.at(output_type)[i]};
            target += coin.adjusted;
            coin_control.Select(coin.outpoint);
        }
    }

    // If automatic coin selection is enabled, add the value of another UTXO to the target
    if (coin_control.m_allow_other_inputs) target += 50 * COIN;
    std::vector<wallet::CRecipient> recipients = {{dest, target, true}};

    bench.epochIterations(5).run([&] {
        LOCK(wallet.cs_wallet);
        const auto& tx_res = CreateTransaction(wallet, recipients, /*refheight=*/chain_size, /*change_pos=*/std::nullopt, coin_control);
        assert(tx_res);
    });
}

static void AvailableCoins(benchmark::Bench& bench, const std::vector<OutputType>& output_type)
{
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();
    // Set clock to genesis block, so the descriptors/keys creation time don't interfere with the blocks scanning process.
    SetMockTime(test_setup->m_node.chainman->GetParams().GenesisBlock().nTime);
    CWallet wallet{test_setup->m_node.chain.get(), "", CreateMockableWalletDatabase()};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetupDescriptorScriptPubKeyMans();
    }

    // Generate destinations
    std::vector<CScript> dest_wallet;
    dest_wallet.reserve(output_type.size());
    for (auto type : output_type) {
        dest_wallet.emplace_back(GetScriptForDestination(getNewDestination(wallet, type)));
    }

    // Generate chain; each coinbase will have two outputs to fill-up the wallet
    const auto& params = Params();
    unsigned int chain_size = 1000;
    for (unsigned int i = 0; i < chain_size / dest_wallet.size(); ++i) {
        for (const auto& dest : dest_wallet) {
            generateFakeBlock(params, test_setup->m_node, wallet, dest);
        }
    }

    // Check available balance
    int next_height = test_setup->m_node.chain->getHeight().value() + 1;
    auto bal = WITH_LOCK(wallet.cs_wallet, return wallet::AvailableCoins(wallet, next_height).GetTotalAmount()); // Cache
    CAmount total{0};
    for (unsigned int height = 0; height < chain_size - COINBASE_MATURITY; ++height) {
        total += GetTimeAdjustedValue(49 * COIN, next_height - height);
        total += GetTimeAdjustedValue( 1 * COIN, next_height - height);
    }
    assert(bal == total);

    bench.epochIterations(2).run([&] {
        LOCK(wallet.cs_wallet);
        const auto& res = wallet::AvailableCoins(wallet, next_height);
        assert(res.All().size() == (chain_size - COINBASE_MATURITY) * 2);
    });
}

static void WalletCreateTxUseOnlyPresetInputs(benchmark::Bench& bench) { WalletCreateTx(bench, OutputType::BECH32, /*allow_other_inputs=*/false,
                                                                                        {{/*num_of_internal_inputs=*/4}}); }

static void WalletCreateTxUsePresetInputsAndCoinSelection(benchmark::Bench& bench) { WalletCreateTx(bench, OutputType::BECH32, /*allow_other_inputs=*/true,
                                                                                                    {{/*num_of_internal_inputs=*/4}}); }

static void WalletAvailableCoins(benchmark::Bench& bench) { AvailableCoins(bench, {OutputType::BECH32}); }

BENCHMARK(WalletCreateTxUseOnlyPresetInputs, benchmark::PriorityLevel::LOW)
BENCHMARK(WalletCreateTxUsePresetInputsAndCoinSelection, benchmark::PriorityLevel::LOW)
BENCHMARK(WalletAvailableCoins, benchmark::PriorityLevel::LOW);
