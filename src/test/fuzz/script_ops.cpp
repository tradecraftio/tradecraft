// Copyright (c) 2020 The Bitcoin Core developers
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

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CScript script = ConsumeScript(fuzzed_data_provider);
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange(0, 7)) {
        case 0:
            script += ConsumeScript(fuzzed_data_provider);
            break;
        case 1:
            script = script + ConsumeScript(fuzzed_data_provider);
            break;
        case 2:
            script << fuzzed_data_provider.ConsumeIntegral<int64_t>();
            break;
        case 3:
            script << ConsumeOpcodeType(fuzzed_data_provider);
            break;
        case 4:
            script << ConsumeScriptNum(fuzzed_data_provider);
            break;
        case 5:
            script << ConsumeRandomLengthByteVector(fuzzed_data_provider);
            break;
        case 6:
            script.clear();
            break;
        case 7: {
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
            break;
        }
        }
    }
}
