// Copyright (c) 2019-2022 The Bitcoin Core developers
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
#include <core_io.h>
#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <univalue.h>

void initialize_script_format()
{
    SelectParams(CBaseChainParams::REGTEST);
}

FUZZ_TARGET_INIT(script_format, initialize_script_format)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CScript script{ConsumeScript(fuzzed_data_provider)};

    (void)FormatScript(script);
    (void)ScriptToAsmStr(script, /*fAttemptSighashDecode=*/fuzzed_data_provider.ConsumeBool());

    UniValue o1(UniValue::VOBJ);
    ScriptPubKeyToUniv(script, o1, /*include_hex=*/fuzzed_data_provider.ConsumeBool());
    UniValue o3(UniValue::VOBJ);
    ScriptToUniv(script, o3);
}
