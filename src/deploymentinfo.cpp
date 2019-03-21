// Copyright (c) 2016-2021 The Bitcoin Core developers
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

#include <deploymentinfo.h>

#include <consensus/params.h>

#include <string_view>

const struct VBDeploymentInfo VersionBitsDeploymentInfo[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] = {
    {
        /*.name =*/ "testdummy",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/ "taproot",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/ "finaltx",
        /*.gbt_force =*/ true,
    },
};

std::string DeploymentName(Consensus::BuriedDeployment dep)
{
    assert(ValidDeployment(dep));
    switch (dep) {
    case Consensus::DEPLOYMENT_HEIGHTINCB:
        return "bip34";
    case Consensus::DEPLOYMENT_DERSIG:
        return "bip66";
    case Consensus::DEPLOYMENT_LOCKTIME:
        return "locktime";
    case Consensus::DEPLOYMENT_SEGWIT:
        return "segwit";
    } // no default case, so the compiler can warn about missing cases
    return "";
}

std::optional<Consensus::BuriedDeployment> GetBuriedDeployment(const std::string_view name)
{
    if (name == "segwit") {
        return Consensus::BuriedDeployment::DEPLOYMENT_SEGWIT;
    } else if (name == "bip34") {
        return Consensus::BuriedDeployment::DEPLOYMENT_HEIGHTINCB;
    } else if (name == "dersig") {
        return Consensus::BuriedDeployment::DEPLOYMENT_DERSIG;
    } else if (name == "locktime") {
        return Consensus::BuriedDeployment::DEPLOYMENT_LOCKTIME;
    }
    return std::nullopt;
}
