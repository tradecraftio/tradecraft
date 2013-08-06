// Copyright (c) 2019 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#ifndef FREICOIN_TEST_UTIL_H
#define FREICOIN_TEST_UTIL_H

#include <memory>
#include <string>

class CBlock;
class CScript;
class CTxIn;
class CWallet;

// Constants //

extern const std::string ADDRESS_BCRT1_UNSPENDABLE;

// Lower-level utils //

/** Returns the generated coin */
std::pair<CTxIn, uint32_t> MineBlock(const CScript& coinbase_scriptPubKey);
/** Prepare a block to be mined */
std::shared_ptr<CBlock> PrepareBlock(const CScript& coinbase_scriptPubKey);


// RPC-like //

/** Import the address to the wallet */
void importaddress(CWallet& wallet, const std::string& address);
/** Returns a new address from the wallet */
std::string getnewaddress(CWallet& w);
/** Returns the generated coin */
std::pair<CTxIn, uint32_t> generatetoaddress(const std::string& address);

/**
 * Increment a string. Useful to enumerate all fixed length strings with
 * characters in [min_char, max_char].
 */
template <typename CharType, size_t StringLength>
bool NextString(CharType (&string)[StringLength], CharType min_char, CharType max_char)
{
    for (CharType& elem : string) {
        bool has_next = elem != max_char;
        elem = elem < min_char || elem >= max_char ? min_char : CharType(elem + 1);
        if (has_next) return true;
    }
    return false;
}

/**
 * Iterate over string values and call function for each string without
 * successive duplicate characters.
 */
template <typename CharType, size_t StringLength, typename Fn>
void ForEachNoDup(CharType (&string)[StringLength], CharType min_char, CharType max_char, Fn&& fn) {
    for (bool has_next = true; has_next; has_next = NextString(string, min_char, max_char)) {
        int prev = -1;
        bool skip_string = false;
        for (CharType c : string) {
            if (c == prev) skip_string = true;
            if (skip_string || c < min_char || c > max_char) break;
            prev = c;
        }
        if (!skip_string) fn();
    }
}

#endif // FREICOIN_TEST_UTIL_H
