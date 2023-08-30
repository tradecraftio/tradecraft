// Copyright (c) 2021-2022 The Bitcoin Core developers
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

#include <node/caches.h>

#include <index/txindex.h>
#include <txdb.h>
#include <util/system.h>

namespace node {
CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes)
{
    int64_t nTotalCache = (args.GetIntArg("-dbcache", nDefaultDbCache) << 20);
    nTotalCache = std::max(nTotalCache, nMinDbCache << 20); // total cache cannot be less than nMinDbCache
    nTotalCache = std::min(nTotalCache, nMaxDbCache << 20); // total cache cannot be greater than nMaxDbcache
    CacheSizes sizes;
    sizes.block_tree_db = std::min(nTotalCache / 8, nMaxBlockDBCache << 20);
    nTotalCache -= sizes.block_tree_db;
    sizes.tx_index = std::min(nTotalCache / 8, args.GetBoolArg("-txindex", DEFAULT_TXINDEX) ? nMaxTxIndexCache << 20 : 0);
    nTotalCache -= sizes.tx_index;
    sizes.filter_index = 0;
    if (n_indexes > 0) {
        int64_t max_cache = std::min(nTotalCache / 8, max_filter_index_cache << 20);
        sizes.filter_index = max_cache / n_indexes;
        nTotalCache -= sizes.filter_index * n_indexes;
    }
    sizes.coins_db = std::min(nTotalCache / 2, (nTotalCache / 4) + (1 << 23)); // use 25%-50% of the remainder for disk cache
    sizes.coins_db = std::min(sizes.coins_db, nMaxCoinsDBCache << 20); // cap total coins db cache
    nTotalCache -= sizes.coins_db;
    sizes.coins = nTotalCache; // the rest goes to in-memory cache
    return sizes;
}
} // namespace node
