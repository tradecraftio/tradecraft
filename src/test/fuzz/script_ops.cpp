// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(script_ops)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CScript script_mut = ConsumeScript(fuzzed_data_provider);
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 1000000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CScript s = ConsumeScript(fuzzed_data_provider);
                script_mut = std::move(s);
            },
            [&] {
                const CScript& s = ConsumeScript(fuzzed_data_provider);
                script_mut = s;
            },
            [&] {
                script_mut << fuzzed_data_provider.ConsumeIntegral<int64_t>();
            },
            [&] {
                script_mut << ConsumeOpcodeType(fuzzed_data_provider);
            },
            [&] {
                script_mut << ConsumeScriptNum(fuzzed_data_provider);
            },
            [&] {
                script_mut << ConsumeRandomLengthByteVector(fuzzed_data_provider);
            },
            [&] {
                script_mut.clear();
            });
    }
    const CScript& script = script_mut;
    (void)script.GetSigOpCount(false);
    (void)script.GetSigOpCount(true);
    (void)script.GetSigOpCount(script);
    (void)script.HasValidOps();
    (void)script.IsPayToScriptHash();
    (void)script.IsPayToWitnessScriptHash();
    (void)script.IsPushOnly();
    (void)script.IsUnspendable();
    {
        CScript::const_iterator pc = script.begin();
        opcodetype opcode;
        (void)script.GetOp(pc, opcode);
        std::vector<uint8_t> data;
        (void)script.GetOp(pc, opcode, data);
        (void)script.IsPushOnly(pc);
    }
    {
        int version;
        std::vector<uint8_t> program;
        (void)script.IsWitnessProgram(version, program);
    }
}
