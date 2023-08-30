// Copyright (c) 2019-2020 The Bitcoin Core developers
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

#ifndef FREICOIN_SCRIPT_KEYORIGIN_H
#define FREICOIN_SCRIPT_KEYORIGIN_H

#include <serialize.h>
#include <vector>

struct KeyOriginInfo
{
    unsigned char fingerprint[4]; //!< First 32 bits of the Hash160 of the public key at the root of the path
    std::vector<uint32_t> path;

    friend bool operator==(const KeyOriginInfo& a, const KeyOriginInfo& b)
    {
        return std::equal(std::begin(a.fingerprint), std::end(a.fingerprint), std::begin(b.fingerprint)) && a.path == b.path;
    }

    friend bool operator<(const KeyOriginInfo& a, const KeyOriginInfo& b)
    {
        // Compare the fingerprints lexicographically
        int fpr_cmp = memcmp(a.fingerprint, b.fingerprint, 4);
        if (fpr_cmp < 0) {
            return true;
        } else if (fpr_cmp > 0) {
            return false;
        }
        // Compare the sizes of the paths, shorter is "less than"
        if (a.path.size() < b.path.size()) {
            return true;
        } else if (a.path.size() > b.path.size()) {
            return false;
        }
        // Paths same length, compare them lexicographically
        return a.path < b.path;
    }

    SERIALIZE_METHODS(KeyOriginInfo, obj) { READWRITE(obj.fingerprint, obj.path); }

    void clear()
    {
        memset(fingerprint, 0, 4);
        path.clear();
    }
};

#endif // FREICOIN_SCRIPT_KEYORIGIN_H
