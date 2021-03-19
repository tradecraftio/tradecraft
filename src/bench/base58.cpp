// Copyright (c) 2016 the Freicoin developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include "bench.h"

#include "main.h"
#include "base58.h"

#include <vector>
#include <string>


static void Base58Encode(benchmark::State& state)
{
    unsigned char buff[32] = {
        17, 79, 8, 99, 150, 189, 208, 162, 22, 23, 203, 163, 36, 58, 147,
        227, 139, 2, 215, 100, 91, 38, 11, 141, 253, 40, 117, 21, 16, 90,
        200, 24
    };
    unsigned char* b = buff;
    while (state.KeepRunning()) {
        EncodeBase58(b, b + 32);
    }
}


static void Base58CheckEncode(benchmark::State& state)
{
    unsigned char buff[32] = {
        17, 79, 8, 99, 150, 189, 208, 162, 22, 23, 203, 163, 36, 58, 147,
        227, 139, 2, 215, 100, 91, 38, 11, 141, 253, 40, 117, 21, 16, 90,
        200, 24
    };
    unsigned char* b = buff;
    std::vector<unsigned char> vch;
    vch.assign(b, b + 32);
    while (state.KeepRunning()) {
        EncodeBase58Check(vch);
    }
}


static void Base58Decode(benchmark::State& state)
{
    const char* addr = "17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem";
    std::vector<unsigned char> vch;
    while (state.KeepRunning()) {
        DecodeBase58(addr, vch);
    }
}


BENCHMARK(Base58Encode);
BENCHMARK(Base58CheckEncode);
BENCHMARK(Base58Decode);
