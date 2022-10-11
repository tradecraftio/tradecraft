// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2011-2022 The Freicoin Developers
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
#include <core_io.h>
#include <rpc/client.h>
#include <rpc/util.h>
#include <test/fuzz/fuzz.h>
#include <util/memory.h>

#include <limits>
#include <string>

void initialize()
{
    static const ECCVerifyHandle verify_handle;
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string random_string(buffer.begin(), buffer.end());
    bool valid = true;
    const UniValue univalue = [&] {
        try {
            return ParseNonRFCJSONValue(random_string);
        } catch (const std::runtime_error&) {
            valid = false;
            return NullUniValue;
        }
    }();
    if (!valid) {
        return;
    }
    try {
        (void)ParseHashO(univalue, "A");
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHashO(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHashV(univalue, "A");
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHashV(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHexO(univalue, "A");
    } catch (const UniValue&) {
    }
    try {
        (void)ParseHexO(univalue, random_string);
    } catch (const UniValue&) {
    }
    try {
        (void)ParseHexUV(univalue, "A");
        (void)ParseHexUV(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHexV(univalue, "A");
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHexV(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseSighashString(univalue);
    } catch (const std::runtime_error&) {
    }
    try {
        (void)AmountFromValue(univalue);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        FlatSigningProvider provider;
        (void)EvalDescriptorStringOrObject(univalue, provider);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseConfirmTarget(univalue, std::numeric_limits<unsigned int>::max());
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseDescriptorRange(univalue);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
}
