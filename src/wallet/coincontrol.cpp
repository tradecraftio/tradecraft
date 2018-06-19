// Copyright (c) 2018-2021 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <wallet/coincontrol.h>

#include <common/args.h>

namespace wallet {
CCoinControl::CCoinControl()
{
    m_avoid_partial_spends = gArgs.GetBoolArg("-avoidpartialspends", DEFAULT_AVOIDPARTIALSPENDS);
}

bool CCoinControl::HasSelected() const
{
    return !m_selected.empty();
}

bool CCoinControl::IsSelected(const COutPoint& outpoint) const
{
    return m_selected.count(outpoint) > 0;
}

bool CCoinControl::IsExternalSelected(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    return it != m_selected.end() && it->second.HasSpentOutput();
}

std::optional<SpentOutput> CCoinControl::GetExternalOutput(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    if (it == m_selected.end() || !it->second.HasSpentOutput()) {
        return std::nullopt;
    }
    return it->second.GetSpentOutput();
}

PreselectedInput& CCoinControl::Select(const COutPoint& outpoint)
{
    auto& input = m_selected[outpoint];
    input.SetPosition(m_selection_pos);
    ++m_selection_pos;
    return input;
}
void CCoinControl::UnSelect(const COutPoint& outpoint)
{
    m_selected.erase(outpoint);
}

void CCoinControl::UnSelectAll()
{
    m_selected.clear();
}

std::vector<COutPoint> CCoinControl::ListSelected() const
{
    std::vector<COutPoint> outpoints;
    std::transform(m_selected.begin(), m_selected.end(), std::back_inserter(outpoints),
        [](const std::map<COutPoint, PreselectedInput>::value_type& pair) {
            return pair.first;
        });
    return outpoints;
}

void CCoinControl::SetInputWeight(const COutPoint& outpoint, int64_t weight)
{
    m_selected[outpoint].SetInputWeight(weight);
}

std::optional<int64_t> CCoinControl::GetInputWeight(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    return it != m_selected.end() ? it->second.GetInputWeight() : std::nullopt;
}

std::optional<uint32_t> CCoinControl::GetSequence(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    return it != m_selected.end() ? it->second.GetSequence() : std::nullopt;
}

std::pair<std::optional<CScript>, std::optional<CScriptWitness>> CCoinControl::GetScripts(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    return it != m_selected.end() ? m_selected.at(outpoint).GetScripts() : std::make_pair(std::nullopt, std::nullopt);
}

void PreselectedInput::SetSpentOutput(const CTxOut& txout, uint32_t refheight)
{
    m_spent_output = SpentOutput{txout, refheight};
}

SpentOutput PreselectedInput::GetSpentOutput() const
{
    assert(m_spent_output.has_value());
    return m_spent_output.value();
}

bool PreselectedInput::HasSpentOutput() const
{
    return m_spent_output.has_value();
}

void PreselectedInput::SetInputWeight(int64_t weight)
{
    m_weight = weight;
}

std::optional<int64_t> PreselectedInput::GetInputWeight() const
{
    return m_weight;
}

void PreselectedInput::SetSequence(uint32_t sequence)
{
    m_sequence = sequence;
}

std::optional<uint32_t> PreselectedInput::GetSequence() const
{
    return m_sequence;
}

void PreselectedInput::SetScriptSig(const CScript& script)
{
    m_script_sig = script;
}

void PreselectedInput::SetScriptWitness(const CScriptWitness& script_wit)
{
    m_script_witness = script_wit;
}

bool PreselectedInput::HasScripts() const
{
    return m_script_sig.has_value() || m_script_witness.has_value();
}

std::pair<std::optional<CScript>, std::optional<CScriptWitness>> PreselectedInput::GetScripts() const
{
    return {m_script_sig, m_script_witness};
}

void PreselectedInput::SetPosition(unsigned int pos)
{
    m_pos = pos;
}

std::optional<unsigned int> PreselectedInput::GetPosition() const
{
    return m_pos;
}
} // namespace wallet
