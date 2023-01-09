// Copyright (c) 2009-2010 Satoshi Nakamoto
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

#ifndef FREICOIN_TXDB_H
#define FREICOIN_TXDB_H

#include <coins.h>
#include <dbwrapper.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class CBlockFileInfo;
class CBlockIndex;
class uint256;
namespace Consensus {
struct Params;
};
struct bilingual_str;

//! -dbcache default (MiB)
static const int64_t nDefaultDbCache = 450;
//! -dbbatchsize default (bytes)
static const int64_t nDefaultDbBatchSize = 16 << 20;
//! max. -dbcache (MiB)
static const int64_t nMaxDbCache = sizeof(void*) > 4 ? 16384 : 1024;
//! min. -dbcache (MiB)
static const int64_t nMinDbCache = 4;
//! Max memory allocated to block tree DB specific cache, if no -txindex (MiB)
static const int64_t nMaxBlockDBCache = 2;
//! Max memory allocated to block tree DB specific cache, if -txindex (MiB)
// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
static const int64_t nMaxTxIndexCache = 1024;
//! Max memory allocated to all block filter index caches combined in MiB.
static const int64_t max_filter_index_cache = 1024;
//! Max memory allocated to coin DB specific cache (MiB)
static const int64_t nMaxCoinsDBCache = 8;

// Actually declared in validation.cpp; can't include because of circular dependency.
extern RecursiveMutex cs_main;

/** CCoinsView backed by the coin database (chainstate/) */
class CCoinsViewDB final : public CCoinsView
{
protected:
    std::unique_ptr<CDBWrapper> m_db;
    fs::path m_ldb_path;
    bool m_is_memory;
public:
    /**
     * @param[in] ldb_path    Location in the filesystem where leveldb data will be stored.
     */
    explicit CCoinsViewDB(fs::path ldb_path, size_t nCacheSize, bool fMemory, bool fWipe);

    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    BlockFinalTxEntry GetFinalTx() const override;
    bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock, const BlockFinalTxEntry &final_tx) override;
    std::unique_ptr<CCoinsViewCursor> Cursor() const override;

    //! Check if the coin database is in a state that cannot be recovered
    //! in-place, necessitating a complete chain reindex.  This does not check
    //! for all possible reasons a reindex might be required.  In fact it
    //! presently only checks if the v14->v15 coin database upgrade needs to be
    //! done, since that requires a full reindex on Freicoin/Tradecraft.
    bool NeedsReindex();

    //! Attempt to update from an older database format. Returns whether an error occurred.
    bool Upgrade();
    size_t EstimateSize() const override;

    //! Dynamically alter the underlying leveldb cache size.
    void ResizeCache(size_t new_cache_size) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
};

/** Access to the block database (blocks/index/) */
class CBlockTreeDB : public CDBWrapper
{
public:
    explicit CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &info);
    bool ReadLastBlockFile(int &nFile);
    bool WriteReindexing(bool fReindexing);
    void ReadReindexing(bool &fReindexing);
    bool WriteFlag(const std::string &name, bool fValue);
    bool ReadFlag(const std::string &name, bool &fValue);
    bool LoadBlockIndexGuts(const Consensus::Params& consensusParams, std::function<CBlockIndex*(const uint256&)> insertBlockIndex)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};

std::optional<bilingual_str> CheckLegacyTxindex(CBlockTreeDB& block_tree_db);

#endif // FREICOIN_TXDB_H
