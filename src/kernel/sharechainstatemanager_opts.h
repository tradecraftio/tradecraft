// Copyright (c) 2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_SHARECHAINSTATEMANAGER_OPTS_H
#define BITCOIN_KERNEL_SHARECHAINSTATEMANAGER_OPTS_H

#include <util/time.h>

#include <cstdint>
#include <functional>

class CChainParams;
struct ShareChainParams;

namespace kernel {

/**
 * @brief An options struct for `ShareChainstateManager`, more ergonomically
 * referred to as `ShareChainstateManager::Options` due to the
 * using-declaration in `ShareChainstateManager`.
 */
struct ShareChainstateManagerOpts {
    const CChainParams& chainparams;
    const ShareChainParams& sharechainparams;
    const std::function<NodeClock::time_point()> adjusted_time_callback{nullptr};
};

} // namespace kernel

#endif // BITCOIN_KERNEL_SHARECHAINSTATEMANAGER_OPTS_H
