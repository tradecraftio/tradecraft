// Copyright (c) 2017-2018 The Bitcoin Core developers
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

#ifndef FREICOIN_WALLET_COINSELECTION_H
#define FREICOIN_WALLET_COINSELECTION_H

#include <amount.h>
#include <primitives/transaction.h>
#include <random.h>

class CFeeRate;

//! target minimum change amount
static constexpr CAmount MIN_CHANGE{COIN / 100};
//! final minimum change amount after paying for fees
static const CAmount MIN_FINAL_CHANGE = MIN_CHANGE/2;

class CInputCoin {
public:
    CInputCoin(uint32_t atheightIn, CAmount adjustedIn, const CTransactionRef& tx, unsigned int i)
    {
        if (!tx)
            throw std::invalid_argument("tx should not be null");
        if (i >= tx->vout.size())
            throw std::out_of_range("The output index is out of range");

        outpoint = COutPoint(tx->GetHash(), i);
        txout = tx->vout[i];
        refheight = tx->lock_height;
        atheight = atheightIn;
        if (adjustedIn < 0) {
            adjustedIn = tx->GetPresentValueOfOutput(i, atheight);
        }
        adjusted = adjustedIn;
        effective_value = adjustedIn;
    }

    CInputCoin(uint32_t atheightIn, CAmount adjustedIn, const CTransactionRef& tx, unsigned int i, int input_bytes) : CInputCoin(atheightIn, adjustedIn, tx, i)
    {
        m_input_bytes = input_bytes;
    }

    COutPoint outpoint;
    CTxOut txout;
    uint32_t refheight;
    uint32_t atheight;
    CAmount adjusted;
    CAmount effective_value;
    CAmount m_fee{0};
    CAmount m_long_term_fee{0};

    /** Pre-computed estimated size of this output as a fully-signed input in a transaction. Can be -1 if it could not be calculated */
    int m_input_bytes{-1};

    bool operator<(const CInputCoin& rhs) const {
        return outpoint < rhs.outpoint;
    }

    bool operator!=(const CInputCoin& rhs) const {
        return outpoint != rhs.outpoint;
    }

    bool operator==(const CInputCoin& rhs) const {
        return outpoint == rhs.outpoint;
    }
};

struct CoinEligibilityFilter
{
    const uint32_t max_height;
    const int conf_mine;
    const int conf_theirs;
    const uint64_t max_ancestors;
    const uint64_t max_descendants;

    CoinEligibilityFilter(uint32_t max_height, int conf_mine, int conf_theirs, uint64_t max_ancestors) : max_height(max_height), conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_ancestors) {}
    CoinEligibilityFilter(uint32_t max_height, int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants) : max_height(max_height), conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants) {}
};

struct OutputGroup
{
    std::vector<CInputCoin> m_outputs;
    bool m_from_me{true};
    CAmount m_value{0};
    uint32_t m_atheight{0};
    int m_depth{999};
    size_t m_ancestors{0};
    size_t m_descendants{0};
    CAmount effective_value{0};
    CAmount fee{0};
    CAmount long_term_fee{0};

    OutputGroup() {}
    OutputGroup(std::vector<CInputCoin>&& outputs, bool from_me, CAmount value, uint32_t atheight, int depth, size_t ancestors, size_t descendants)
    : m_outputs(std::move(outputs))
    , m_from_me(from_me)
    , m_value(value)
    , m_atheight(atheight)
    , m_depth(depth)
    , m_ancestors(ancestors)
    , m_descendants(descendants)
    {}
    OutputGroup(const CInputCoin& output, int depth, bool from_me, size_t ancestors, size_t descendants) : OutputGroup() {
        Insert(output, depth, from_me, ancestors, descendants);
    }
    void Insert(const CInputCoin& output, int depth, bool from_me, size_t ancestors, size_t descendants);
    std::vector<CInputCoin>::iterator Discard(const CInputCoin& output);
    bool EligibleForSpending(const CoinEligibilityFilter& eligibility_filter) const;

    //! Update the OutputGroup's fee, long_term_fee, and effective_value based on the given feerates
    void SetFees(const CFeeRate effective_feerate, const CFeeRate long_term_feerate);
    OutputGroup GetPositiveOnlyGroup();
};

bool SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& target_value, const CAmount& cost_of_change, std::set<CInputCoin>& out_set, CAmount& value_ret, CAmount not_input_fees);

// Original coin selection algorithm as a fallback
bool KnapsackSolver(const CAmount& nTargetValue, std::vector<OutputGroup>& groups, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet);

#endif // FREICOIN_WALLET_COINSELECTION_H
