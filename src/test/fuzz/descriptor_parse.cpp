// Copyright (c) 2009-2021 The Bitcoin Core developers
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

#include <chainparams.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <test/fuzz/fuzz.h>

void initialize_descriptor_parse()
{
    ECC_Start();
    SelectParams(CBaseChainParams::MAIN);
}

FUZZ_TARGET_INIT(descriptor_parse, initialize_descriptor_parse)
{
    const std::string descriptor(buffer.begin(), buffer.end());
    FlatSigningProvider signing_provider;
    std::string error;
    for (const bool require_checksum : {true, false}) {
        const auto desc = Parse(descriptor, signing_provider, error, require_checksum);
        if (desc) {
            (void)desc->ToString();
            (void)desc->IsRange();
            (void)desc->IsSolvable();
        }
    }
}
