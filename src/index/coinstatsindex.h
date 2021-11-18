// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#ifndef FREICOIN_INDEX_COINSTATSINDEX_H
#define FREICOIN_INDEX_COINSTATSINDEX_H

#include <crypto/muhash.h>
#include <index/base.h>

class CBlockIndex;
class CDBBatch;
namespace kernel {
struct CCoinsStats;
}

static constexpr bool DEFAULT_COINSTATSINDEX{false};

/**
 * CoinStatsIndex maintains statistics on the UTXO set.
 */
class CoinStatsIndex final : public BaseIndex
{
private:
    std::unique_ptr<BaseIndex::DB> m_db;

    MuHash3072 m_muhash;
    uint64_t m_transaction_output_count{0};
    uint64_t m_bogo_size{0};
    CAmount m_total_value{0};
    CAmount m_total_subsidy{0};
    CAmount m_total_unspendable_value{0};
    CAmount m_total_prevout_spent_amount{0};
    CAmount m_total_new_outputs_ex_coinbase_amount{0};
    CAmount m_total_coinbase_amount{0};
    CAmount m_total_unspendables_genesis_block{0};
    CAmount m_total_unspendables_bip30{0};
    CAmount m_total_unspendables_scripts{0};
    CAmount m_total_unspendables_unclaimed_rewards{0};

    bool ReverseBlock(const CBlock& block, const CBlockIndex* pindex);

    bool AllowPrune() const override { return true; }

protected:
    bool CustomInit(const std::optional<interfaces::BlockKey>& block) override;

    bool CustomCommit(CDBBatch& batch) override;

    bool CustomAppend(const interfaces::BlockInfo& block) override;

    bool CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip) override;

    BaseIndex::DB& GetDB() const override { return *m_db; }

public:
    // Constructs the index, which becomes available to be queried.
    explicit CoinStatsIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    // Look up stats for a specific block using CBlockIndex
    std::optional<kernel::CCoinsStats> LookUpStats(const CBlockIndex& block_index) const;
};

/// The global UTXO set hash object.
extern std::unique_ptr<CoinStatsIndex> g_coin_stats_index;

#endif // FREICOIN_INDEX_COINSTATSINDEX_H
