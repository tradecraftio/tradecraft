// Copyright (c) 2021-2022 The Bitcoin Core developers
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

#include <consensus/amount.h>
#include <policy/fees.h>
#include <script/solver.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(spend_tests, WalletTestingSetup)

BOOST_FIXTURE_TEST_CASE(SubtractFee, TestChain100Setup)
{
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    auto wallet = CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey);
    {
        LOCK(wallet->cs_wallet);
        BOOST_CHECK_EQUAL(AvailableCoins(*wallet, /*atheight=*/0).All().size(), 0);
        BOOST_CHECK_EQUAL(AvailableCoins(*wallet, /*atheight=*/1).All().size(), 0);
        BOOST_CHECK_EQUAL(AvailableCoins(*wallet, /*atheight=*/2).All().size(), 1);
    }

    // Check that a subtract-from-recipient transaction slightly less than the
    // coinbase input amount does not create a change output (because it would
    // be uneconomical to add and spend the output), and make sure it pays the
    // leftover input amount which would have been change to the recipient
    // instead of the miner.
    auto check_tx = [&wallet](CAmount leftover_input_amount) {
        CRecipient recipient{PubKeyDestination({}), 50 * COIN - leftover_input_amount, /*subtract_fee=*/true};
        CCoinControl coin_control;
        coin_control.m_feerate.emplace(10000);
        coin_control.fOverrideFeeRate = true;
        // We need to use a change type with high cost of change so that the leftover amount will be dropped to fee instead of added as a change output
        coin_control.m_change_type = OutputType::LEGACY;
        auto res = CreateTransaction(*wallet, {recipient}, /*refheight=*/std::nullopt, /*change_pos=*/std::nullopt, coin_control);
        BOOST_CHECK(!res); // Demurrage has destroyed the input
        // Fails now that the coinbase lock_height is required to be equal to
        // the block height:
        res = CreateTransaction(*wallet, {recipient}, /*refheight=*/1, /*change_pos=*/std::nullopt, coin_control);
        BOOST_CHECK(!res);
        res = CreateTransaction(*wallet, {recipient}, /*refheight=*/2, /*change_pos=*/std::nullopt, coin_control);
        BOOST_CHECK(res);
        const auto& txr = *res;
        BOOST_CHECK_EQUAL(txr.tx->vout.size(), 1);
        BOOST_CHECK_EQUAL(txr.tx->vout[0].GetReferenceValue(), recipient.nAmount + leftover_input_amount - txr.fee);
        BOOST_CHECK_GT(txr.fee, 0);
        return txr.fee;
    };

    // Send full input amount to recipient, check that only nonzero fee is
    // subtracted (to_reduce == fee).
    const CAmount fee{check_tx(0)};

    // Send slightly less than full input amount to recipient, check leftover
    // input amount is paid to recipient not the miner (to_reduce == fee - 123)
    BOOST_CHECK_EQUAL(fee, check_tx(123));

    // Send full input minus fee amount to recipient, check leftover input
    // amount is paid to recipient not the miner (to_reduce == 0)
    BOOST_CHECK_EQUAL(fee, check_tx(fee));

    // Send full input minus more than the fee amount to recipient, check
    // leftover input amount is paid to recipient not the miner (to_reduce ==
    // -123). This overpays the recipient instead of overpaying the miner more
    // than double the necessary fee.
    BOOST_CHECK_EQUAL(fee, check_tx(fee + 123));
}

BOOST_FIXTURE_TEST_CASE(wallet_duplicated_preset_inputs_test, TestChain100Setup)
{
    // Verify that the wallet's Coin Selection process does not include pre-selected inputs twice in a transaction.

    // Add 4 spendable UTXO, 50 FRC each, to the wallet (total balance 200 FRC)
    for (int i = 0; i < 4; i++) CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    auto wallet = CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey);

    LOCK(wallet->cs_wallet);
    auto available_coins = AvailableCoins(*wallet, m_node.chain->getHeight().value() + 1);
    std::vector<COutput> coins = available_coins.All();
    // Preselect the first 3 UTXO (150 FRC total)
    std::set<COutPoint> preset_inputs = {coins[0].outpoint, coins[1].outpoint, coins[2].outpoint};

    // Try to create a tx that spends more than what preset inputs + wallet selected inputs are covering for.
    // The wallet can cover up to 200 FRC, and the tx target is 299 FRC.
    std::vector<CRecipient> recipients{{*Assert(wallet->GetNewDestination(OutputType::BECH32, "dummy")),
                                           /*nAmount=*/299 * COIN, /*fSubtractFeeFromAmount=*/true}};
    CCoinControl coin_control;
    coin_control.m_allow_other_inputs = true;
    for (const auto& outpoint : preset_inputs) {
        coin_control.Select(outpoint);
    }

    // Attempt to send 299 FRC from a wallet that only has 200 FRC. The wallet should exclude
    // the preset inputs from the pool of available coins, realize that there is not enough
    // money to fund the 299 FRC payment, and fail with "Insufficient funds".
    //
    // Even with SFFO, the wallet can only afford to send 200 FRC.
    // If the wallet does not properly exclude preset inputs from the pool of available coins
    // prior to coin selection, it may create a transaction that does not fund the full payment
    // amount or, through SFFO, incorrectly reduce the recipient's amount by the difference
    // between the original target and the wrongly counted inputs (in this case 99 FRC)
    // so that the recipient's amount is no longer equal to the user's selected target of 299 FRC.

    // First case, use 'subtract_fee_from_outputs=true'
    util::Result<CreatedTransactionResult> res_tx = CreateTransaction(*wallet, recipients, /*refheight=*/1, /*change_pos=*/std::nullopt, coin_control);
    BOOST_CHECK(!res_tx.has_value());

    // Second case, don't use 'subtract_fee_from_outputs'.
    recipients[0].fSubtractFeeFromAmount = false;
    res_tx = CreateTransaction(*wallet, recipients, /*refheight=*/1, /*change_pos=*/std::nullopt, coin_control);
    BOOST_CHECK(!res_tx.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
