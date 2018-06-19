// Copyright (c) 2022 The Bitcoin Core developers
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

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/coincontrol.h>
#include <wallet/test/util.h>

namespace wallet {
namespace {

const TestingSetup* g_setup;

void initialize_coincontrol()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(coincontrol, .init = initialize_coincontrol)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto& node = g_setup->m_node;
    ArgsManager& args = *node.args;

    // for GetBoolArg to return true sometimes
    args.ForceSetArg("-avoidpartialspends", fuzzed_data_provider.ConsumeBool()?"1":"0");

    CCoinControl coin_control;
    COutPoint out_point;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                std::optional<COutPoint> optional_out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
                if (!optional_out_point) {
                    return;
                }
                out_point = *optional_out_point;
            },
            [&] {
                (void)coin_control.HasSelected();
            },
            [&] {
                (void)coin_control.IsSelected(out_point);
            },
            [&] {
                (void)coin_control.IsExternalSelected(out_point);
            },
            [&] {
                (void)coin_control.GetExternalOutput(out_point);
            },
            [&] {
                (void)coin_control.Select(out_point);
            },
            [&] {
                const CTxOut tx_out{ConsumeMoney(fuzzed_data_provider), ConsumeScript(fuzzed_data_provider)};
                const uint32_t refheight = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
                (void)coin_control.SelectExternal(out_point, SpentOutput{tx_out, refheight});
            },
            [&] {
                (void)coin_control.UnSelect(out_point);
            },
            [&] {
                (void)coin_control.UnSelectAll();
            },
            [&] {
                (void)coin_control.ListSelected();
            },
            [&] {
                int64_t weight{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
                (void)coin_control.SetInputWeight(out_point, weight);
            },
            [&] {
                // Condition to avoid the assertion in GetInputWeight
                if (coin_control.HasInputWeight(out_point)) {
                    (void)coin_control.GetInputWeight(out_point);
                }
            });
    }
}
} // namespace
} // namespace wallet
