// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
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

#include <outputtype.h>

#include <keystore.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

#include <assert.h>
#include <string>

static const std::string OUTPUT_TYPE_STRING_LEGACY = "legacy";
static const std::string OUTPUT_TYPE_STRING_BECH32 = "bech32";

bool ParseOutputType(const std::string& type, OutputType& output_type)
{
    if (type == OUTPUT_TYPE_STRING_LEGACY) {
        output_type = OutputType::LEGACY;
        return true;
    } else if (type == OUTPUT_TYPE_STRING_BECH32) {
        output_type = OutputType::BECH32;
        return true;
    }
    return false;
}

const std::string& FormatOutputType(OutputType type)
{
    switch (type) {
    case OutputType::LEGACY: return OUTPUT_TYPE_STRING_LEGACY;
    case OutputType::BECH32: return OUTPUT_TYPE_STRING_BECH32;
    default: assert(false);
    }
}

CTxDestination GetDestinationForKey(const CPubKey& key, OutputType type)
{
    switch (type) {
    case OutputType::LEGACY: return key.GetID();
    case OutputType::BECH32: {
        if (!key.IsCompressed()) return key.GetID();
        CScript p2pk = GetScriptForRawPubKey(key);
        CTxDestination witdest = WitnessV0ShortHash((unsigned char)0, p2pk);
        return witdest;
    }
    default: assert(false);
    }
}

std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey& key)
{
    CKeyID keyid = key.GetID();
    if (key.IsCompressed()) {
        CScript p2pk = GetScriptForRawPubKey(key);
        CTxDestination segwit = WitnessV0ShortHash((unsigned char)0, p2pk);
        return std::vector<CTxDestination>{std::move(keyid), std::move(segwit)};
    } else {
        return std::vector<CTxDestination>{std::move(keyid)};
    }
}

CTxDestination AddAndGetDestinationForScript(CKeyStore& keystore, const CScript& script, OutputType type)
{
    // Add script to keystore
    keystore.AddCScript(script);
    // Note that scripts over 520 bytes are not yet supported.
    switch (type) {
    case OutputType::LEGACY:
        return CScriptID(script);
    case OutputType::BECH32: {
        CTxDestination witdest = WitnessV0LongHash(0 /* version */, script);
        CScript witprog = GetScriptForDestination(witdest);
        WitnessV0ScriptEntry entry(0 /* version */, script);
        keystore.AddWitnessV0Script(entry);
        // Check if the resulting program is solvable (i.e. doesn't use an uncompressed key)
        if (!IsSolvable(keystore, witprog)) return CScriptID(script);
        return witdest;
    }
    default: assert(false);
    }
}

