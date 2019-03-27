// Copyright (c) 2011-2019 The Bitcoin Core developers
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

#ifndef FREICOIN_WALLET_COINCONTROL_H
#define FREICOIN_WALLET_COINCONTROL_H

#include <outputtype.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <script/standard.h>

#include <optional>

const int DEFAULT_MIN_DEPTH = 0;
const int DEFAULT_MAX_DEPTH = 9999999;

//! Default for -avoidpartialspends
static constexpr bool DEFAULT_AVOIDPARTIALSPENDS = false;

/** Coin Control Features. */
class CCoinControl
{
public:
    //! Custom change destination, if not set an address is generated
    CTxDestination destChange = CNoDestination();
    //! Override the default change type if set, ignored if destChange is set
    std::optional<OutputType> m_change_type;
    //! If false, only selected inputs are used
    bool m_add_inputs = true;
    //! If false, only safe inputs will be used
    bool m_include_unsafe_inputs = false;
    //! If false, allows unselected inputs, but requires all selected inputs be used
    bool fAllowOtherInputs = false;
    //! Includes watch only addresses which are solvable
    bool fAllowWatchOnly = false;
    //! Override automatic min/max checks on fee, m_feerate must be set if true
    bool fOverrideFeeRate = false;
    //! Override the wallet's m_pay_tx_fee if set
    std::optional<CFeeRate> m_feerate;
    //! Override the default confirmation target if set
    std::optional<unsigned int> m_confirm_target;
    //! Avoid partial use of funds sent to a given address
    bool m_avoid_partial_spends = DEFAULT_AVOIDPARTIALSPENDS;
    //! Forbids inclusion of dirty (previously used) addresses
    bool m_avoid_address_reuse = false;
    //! Fee estimation mode to control arguments to estimateSmartFee
    FeeEstimateMode m_fee_mode = FeeEstimateMode::UNSET;
    //! Minimum chain depth value for coin availability
    int m_min_depth = DEFAULT_MIN_DEPTH;
    //! Maximum chain depth value for coin availability
    int m_max_depth = DEFAULT_MAX_DEPTH;

    CCoinControl();

    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }

    bool IsSelected(const COutPoint& output) const
    {
        return (setSelected.count(output) > 0);
    }

    void Select(const COutPoint& output)
    {
        setSelected.insert(output);
    }

    void UnSelect(const COutPoint& output)
    {
        setSelected.erase(output);
    }

    void UnSelectAll()
    {
        setSelected.clear();
    }

    void ListSelected(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }

private:
    std::set<COutPoint> setSelected;
};

#endif // FREICOIN_WALLET_COINCONTROL_H
