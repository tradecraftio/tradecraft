// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SHARECHAINTYPE_H
#define BITCOIN_UTIL_SHARECHAINTYPE_H

#include <optional> // for std::optional
#include <string> // for std::string

enum class ShareChainType {
    SOLO,
    MAIN,
};

std::string ShareChainTypeToString(ShareChainType share_chain);

std::optional<ShareChainType> ShareChainTypeFromString(std::string_view share_chain);

#endif // BITCOIN_UTIL_SHARECHAINTYPE_H

// End of File
