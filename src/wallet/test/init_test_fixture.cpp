// Copyright (c) 2018-2022 The Bitcoin Core developers
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

#include <common/args.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <util/fs.h>

#include <fstream>
#include <string>

#include <wallet/test/init_test_fixture.h>

namespace wallet {
InitWalletDirTestingSetup::InitWalletDirTestingSetup(const ChainType chainType) : BasicTestingSetup(chainType)
{
    m_wallet_loader = MakeWalletLoader(*m_node.chain, m_args);

    const auto sep = fs::path::preferred_separator;

    m_datadir = m_args.GetDataDirNet();
    m_cwd = fs::current_path();

    m_walletdir_path_cases["default"] = m_datadir / "wallets";
    m_walletdir_path_cases["custom"] = m_datadir / "my_wallets";
    m_walletdir_path_cases["nonexistent"] = m_datadir / "path_does_not_exist";
    m_walletdir_path_cases["file"] = m_datadir / "not_a_directory.dat";
    m_walletdir_path_cases["trailing"] = (m_datadir / "wallets") + sep;
    m_walletdir_path_cases["trailing2"] = (m_datadir / "wallets") + sep + sep;

    fs::current_path(m_datadir);
    m_walletdir_path_cases["relative"] = "wallets";

    fs::create_directories(m_walletdir_path_cases["default"]);
    fs::create_directories(m_walletdir_path_cases["custom"]);
    fs::create_directories(m_walletdir_path_cases["relative"]);
    std::ofstream f{m_walletdir_path_cases["file"]};
    f.close();
}

InitWalletDirTestingSetup::~InitWalletDirTestingSetup()
{
    fs::current_path(m_cwd);
}

void InitWalletDirTestingSetup::SetWalletDir(const fs::path& walletdir_path)
{
    m_args.ForceSetArg("-walletdir", fs::PathToString(walletdir_path));
}
} // namespace wallet
