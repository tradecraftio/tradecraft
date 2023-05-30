// Copyright (c) 2020-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/sharechaintype.h>

#include <cassert> // for assert
#include <optional> // for std::nullopt, std::optional
#include <string> // for std::string

std::string ShareChainTypeToString(ShareChainType share_chain)
{
    switch (share_chain) {
    case ShareChainType::SOLO:
        return "solo";
    case ShareChainType::MAIN:
        return "main";
    }
    assert(false);
}

std::optional<ShareChainType> ShareChainTypeFromString(std::string_view share_chain)
{
    if (share_chain == "solo") {
        return ShareChainType::SOLO;
    } else if (share_chain == "main") {
        return ShareChainType::MAIN;
    }
    return std::nullopt;
}

// End of File
