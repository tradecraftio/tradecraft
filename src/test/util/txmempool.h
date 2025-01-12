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

#ifndef FREICOIN_TEST_UTIL_TXMEMPOOL_H
#define FREICOIN_TEST_UTIL_TXMEMPOOL_H

#include <policy/packages.h>
#include <txmempool.h>
#include <util/time.h>

namespace node {
struct NodeContext;
}
struct PackageMempoolAcceptResult;

CTxMemPool::Options MemPoolOptionsForTest(const node::NodeContext& node);

struct TestMemPoolEntryHelper {
    // Default values
    CAmount nFee{0};
    NodeSeconds time{};
    unsigned int nHeight{1};
    uint64_t m_sequence{0};
    bool spendsCoinbase{false};
    unsigned int sigOpCost{4};
    LockPoints lp;

    CTxMemPoolEntry FromTx(const CMutableTransaction& tx) const;
    CTxMemPoolEntry FromTx(const CTransactionRef& tx) const;

    // Change the default value
    TestMemPoolEntryHelper& Fee(CAmount _fee) { nFee = _fee; return *this; }
    TestMemPoolEntryHelper& Time(NodeSeconds tp) { time = tp; return *this; }
    TestMemPoolEntryHelper& Height(unsigned int _height) { nHeight = _height; return *this; }
    TestMemPoolEntryHelper& Sequence(uint64_t _seq) { m_sequence = _seq; return *this; }
    TestMemPoolEntryHelper& SpendsCoinbase(bool _flag) { spendsCoinbase = _flag; return *this; }
    TestMemPoolEntryHelper& SigOpsCost(unsigned int _sigopsCost) { sigOpCost = _sigopsCost; return *this; }
};

/** Check expected properties for every PackageMempoolAcceptResult, regardless of value. Returns
 * a string if an error occurs with error populated, nullopt otherwise. If mempool is provided,
 * checks that the expected transactions are in mempool (this should be set to nullptr for a test_accept).
*/
std::optional<std::string>  CheckPackageMempoolAcceptResult(const Package& txns,
                                                            const PackageMempoolAcceptResult& result,
                                                            bool expect_valid,
                                                            const CTxMemPool* mempool);

/** For every transaction in tx_pool, check TRUC invariants:
 * - a TRUC tx's ancestor count must be within TRUC_ANCESTOR_LIMIT
 * - a TRUC tx's descendant count must be within TRUC_DESCENDANT_LIMIT
 * - if a TRUC tx has ancestors, its sigop-adjusted vsize must be within TRUC_CHILD_MAX_VSIZE
 * - any non-TRUC tx must only have non-TRUC parents
 * - any TRUC tx must only have TRUC parents
 *   */
void CheckMempoolTRUCInvariants(const CTxMemPool& tx_pool);

#endif // FREICOIN_TEST_UTIL_TXMEMPOOL_H
