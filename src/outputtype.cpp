// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#include <outputtype.h>

#include <pubkey.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <util/vector.h>

#include <assert.h>
#include <optional>
#include <string>

static const std::string OUTPUT_TYPE_STRING_LEGACY = "legacy";
static const std::string OUTPUT_TYPE_STRING_BECH32 = "bech32";
static const std::string OUTPUT_TYPE_STRING_UNKNOWN = "unknown";

std::optional<OutputType> ParseOutputType(const std::string& type)
{
    if (type == OUTPUT_TYPE_STRING_LEGACY) {
        return OutputType::LEGACY;
    } else if (type == OUTPUT_TYPE_STRING_BECH32) {
        return OutputType::BECH32;
    }
    return std::nullopt;
}

const std::string& FormatOutputType(OutputType type)
{
    switch (type) {
    case OutputType::LEGACY: return OUTPUT_TYPE_STRING_LEGACY;
    case OutputType::BECH32: return OUTPUT_TYPE_STRING_BECH32;
    case OutputType::UNKNOWN: return OUTPUT_TYPE_STRING_UNKNOWN;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

CTxDestination GetDestinationForKey(const CPubKey& key, OutputType type)
{
    switch (type) {
    case OutputType::LEGACY: return PKHash(key);
    case OutputType::BECH32: {
        if (!key.IsCompressed()) return PKHash(key);
        return WitnessV0ShortHash(0 /* version */, key);
    }
    case OutputType::UNKNOWN: {} // This function should never be used with UNKNOWN, so let it assert
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey& key)
{
    PKHash keyid(key);
    CTxDestination p2pkh{keyid};
    if (key.IsCompressed()) {
        CTxDestination segwit = WitnessV0ShortHash(0 /* version */, key);
        return Vector(std::move(p2pkh), std::move(segwit));
    } else {
        return Vector(std::move(p2pkh));
    }
}

CTxDestination AddAndGetDestinationForScript(FillableSigningProvider& keystore, const CScript& script, OutputType type)
{
    // Add script to keystore
    keystore.AddCScript(script);
    // Note that scripts over 520 bytes are not yet supported.
    switch (type) {
    case OutputType::LEGACY:
        return ScriptHash(script);
    case OutputType::BECH32: {
        WitnessV0ScriptEntry entry(/*version=*/0, script);
        keystore.AddWitnessV0Script(entry);
        return entry.GetLongHash();
    }
    case OutputType::UNKNOWN: {} // This function should not be used for UNKNOWN, so let it assert
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

std::optional<OutputType> OutputTypeFromDestination(const CTxDestination& dest) {
    if (std::holds_alternative<PKHash>(dest) ||
        std::holds_alternative<ScriptHash>(dest)) {
        return OutputType::LEGACY;
    }
    if (std::holds_alternative<WitnessV0ShortHash>(dest) ||
        std::holds_alternative<WitnessV0LongHash>(dest) ||
        std::holds_alternative<WitnessUnknown>(dest)) {
        return OutputType::BECH32;
    }
    return std::nullopt;
}
