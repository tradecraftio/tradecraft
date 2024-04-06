// Copyright (c) 2019-2021 The Bitcoin Core developers
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

#ifndef BITCOIN_NET_TYPES_H
#define BITCOIN_NET_TYPES_H

#include <cstdint>
#include <map>

class CSubNet;
class UniValue;

class CBanEntry
{
public:
    static constexpr int CURRENT_VERSION{1};
    int nVersion{CBanEntry::CURRENT_VERSION};
    int64_t nCreateTime{0};
    int64_t nBanUntil{0};

    CBanEntry() {}

    explicit CBanEntry(int64_t nCreateTimeIn)
        : nCreateTime{nCreateTimeIn} {}

    /**
     * Create a ban entry from JSON.
     * @param[in] json A JSON representation of a ban entry, as created by `ToJson()`.
     * @throw std::runtime_error if the JSON does not have the expected fields.
     */
    explicit CBanEntry(const UniValue& json);

    /**
     * Generate a JSON representation of this ban entry.
     * @return JSON suitable for passing to the `CBanEntry(const UniValue&)` constructor.
     */
    UniValue ToJson() const;
};

using banmap_t = std::map<CSubNet, CBanEntry>;

/**
 * Convert a `banmap_t` object to a JSON array.
 * @param[in] bans Bans list to convert.
 * @return a JSON array, similar to the one returned by the `listbanned` RPC. Suitable for
 * passing to `BanMapFromJson()`.
 */
UniValue BanMapToJson(const banmap_t& bans);

/**
 * Convert a JSON array to a `banmap_t` object.
 * @param[in] bans_json JSON to convert, must be as returned by `BanMapToJson()`.
 * @param[out] bans Bans list to create from the JSON.
 * @throws std::runtime_error if the JSON does not have the expected fields or they contain
 * unparsable values.
 */
void BanMapFromJson(const UniValue& bans_json, banmap_t& bans);

#endif // BITCOIN_NET_TYPES_H
