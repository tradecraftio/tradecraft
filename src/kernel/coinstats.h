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

#ifndef FREICOIN_KERNEL_COINSTATS_H
#define FREICOIN_KERNEL_COINSTATS_H

#include <consensus/amount.h>
#include <streams.h>
#include <uint256.h>

#include <cstdint>
#include <functional>
#include <optional>

class CCoinsView;
class Coin;
class COutPoint;
class CScript;
namespace node {
class BlockManager;
} // namespace node

namespace kernel {
enum class CoinStatsHashType {
    HASH_SERIALIZED,
    MUHASH,
    NONE,
};

struct CCoinsStats {
    int nHeight{0};
    uint256 hashBlock{};
    uint64_t nTransactions{0};
    uint64_t nTransactionOutputs{0};
    uint64_t nBogoSize{0};
    uint256 hashSerialized{};
    uint64_t nDiskSize{0};
    //! The total amount, or nullopt if an overflow occurred calculating it
    std::optional<CAmount> total_amount{0};

    //! The number of coins contained.
    uint64_t coins_count{0};

    //! Signals if the coinstatsindex was used to retrieve the statistics.
    bool index_used{false};

    // Following values are only available from coinstats index

    //! Total cumulative amount of block subsidies up to and including this block
    CAmount total_subsidy{0};
    //! Total cumulative amount of unspendable coins up to and including this block
    CAmount total_unspendable_amount{0};
    //! Total cumulative amount of prevouts spent up to and including this block
    CAmount total_prevout_spent_amount{0};
    //! Total cumulative amount of outputs created up to and including this block
    CAmount total_new_outputs_ex_coinbase_amount{0};
    //! Total cumulative amount of coinbase outputs up to and including this block
    CAmount total_coinbase_amount{0};
    //! The unspendable coinbase amount from the genesis block
    CAmount total_unspendables_genesis_block{0};
    //! The two unspendable coinbase outputs total amount caused by BIP30
    CAmount total_unspendables_bip30{0};
    //! Total cumulative amount of outputs sent to unspendable scripts (OP_RETURN for example) up to and including this block
    CAmount total_unspendables_scripts{0};
    //! Total cumulative amount of coins lost due to unclaimed miner rewards up to and including this block
    CAmount total_unspendables_unclaimed_rewards{0};

    CCoinsStats() = default;
    CCoinsStats(int block_height, const uint256& block_hash);
};

uint64_t GetBogoSize(const CScript& script_pub_key);

DataStream TxOutSer(const COutPoint& outpoint, const Coin& coin);

std::optional<CCoinsStats> ComputeUTXOStats(CoinStatsHashType hash_type, CCoinsView* view, node::BlockManager& blockman, const std::function<void()>& interruption_point = {});
} // namespace kernel

#endif // FREICOIN_KERNEL_COINSTATS_H
