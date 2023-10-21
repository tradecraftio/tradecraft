// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
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

#include <uint256.h>

#include <util/strencodings.h>

template <unsigned int BITS>
std::string base_blob<BITS>::GetHex() const
{
    uint8_t m_data_rev[WIDTH];
    for (int i = 0; i < WIDTH; ++i) {
        m_data_rev[i] = m_data[WIDTH - 1 - i];
    }
    return HexStr(m_data_rev);
}

template <unsigned int BITS>
void base_blob<BITS>::SetHex(const char* psz)
{
    std::fill(m_data.begin(), m_data.end(), 0);

    // skip leading spaces
    while (IsSpace(*psz))
        psz++;

    // skip 0x
    if (psz[0] == '0' && ToLower(psz[1]) == 'x')
        psz += 2;

    // hex string to uint
    size_t digits = 0;
    while (::HexDigit(psz[digits]) != -1)
        digits++;
    unsigned char* p1 = m_data.data();
    unsigned char* pend = p1 + WIDTH;
    while (digits > 0 && p1 < pend) {
        *p1 = ::HexDigit(psz[--digits]);
        if (digits > 0) {
            *p1 |= ((unsigned char)::HexDigit(psz[--digits]) << 4);
            p1++;
        }
    }
}

template <unsigned int BITS>
void base_blob<BITS>::SetHex(const std::string& str)
{
    SetHex(str.c_str());
}

template <unsigned int BITS>
std::string base_blob<BITS>::ToString() const
{
    return (GetHex());
}

// Explicit instantiations for base_blob<160>
template std::string base_blob<160>::GetHex() const;
template std::string base_blob<160>::ToString() const;
template void base_blob<160>::SetHex(const char*);
template void base_blob<160>::SetHex(const std::string&);

// Explicit instantiations for base_blob<256>
template std::string base_blob<256>::GetHex() const;
template std::string base_blob<256>::ToString() const;
template void base_blob<256>::SetHex(const char*);
template void base_blob<256>::SetHex(const std::string&);

const uint256 uint256::ZERO(0);
const uint256 uint256::ONE(1);
