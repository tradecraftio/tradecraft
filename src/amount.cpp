// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2011-2019 The Freicoin developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include "amount.h"

#include "tinyformat.h"

CFeeRate::CFeeRate(const CAmount& nFeePaid, size_t nSize)
{
    if (nSize > 0)
        nSatoshisPerK = nFeePaid*1000/nSize;
    else
        nSatoshisPerK = 0;
}

CAmount CFeeRate::GetFee(size_t nSize) const
{
    CAmount nFee = nSatoshisPerK*nSize / 1000;

    if (nFee == 0 && nSatoshisPerK > 0)
        nFee = nSatoshisPerK;

    return nFee;
}

std::string CFeeRate::ToString() const
{
    return strprintf("%d.%08d BTC/kB", nSatoshisPerK / COIN, nSatoshisPerK % COIN);
}
