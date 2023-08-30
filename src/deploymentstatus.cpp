// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#include <deploymentstatus.h>

#include <consensus/params.h>
#include <versionbits.h>

#include <type_traits>

/* Basic sanity checking for BuriedDeployment/DeploymentPos enums and
 * ValidDeployment check */

static_assert(ValidDeployment(Consensus::DEPLOYMENT_TESTDUMMY), "sanity check of DeploymentPos failed (TESTDUMMY not valid)");
static_assert(!ValidDeployment(Consensus::MAX_VERSION_BITS_DEPLOYMENTS), "sanity check of DeploymentPos failed (MAX value considered valid)");
static_assert(!ValidDeployment(static_cast<Consensus::BuriedDeployment>(Consensus::DEPLOYMENT_TESTDUMMY)), "sanity check of BuriedDeployment failed (overlaps with DeploymentPos)");

/* ValidDeployment only checks upper bounds for ensuring validity.
 * This checks that the lowest possible value or the type is also a
 * (specific) valid deployment so that lower bounds don't need to be checked.
 */

template<typename T, T x>
static constexpr bool is_minimum()
{
    using U = typename std::underlying_type<T>::type;
    return x == std::numeric_limits<U>::min();
}

static_assert(is_minimum<Consensus::BuriedDeployment, Consensus::DEPLOYMENT_HEIGHTINCB>(), "heightincb is not minimum value for BuriedDeployment");
static_assert(is_minimum<Consensus::DeploymentPos, Consensus::DEPLOYMENT_TESTDUMMY>(), "testdummy is not minimum value for DeploymentPos");
