// Copyright (c) 2018-2021 The Bitcoin Core developers
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

#include <fs.h>
#include <univalue.h>
#include <util/check.h>
#include <util/system.h>

#include <fstream>
#include <string>

#include <wallet/test/init_test_fixture.h>

namespace wallet {
InitWalletDirTestingSetup::InitWalletDirTestingSetup(const std::string& chainName) : BasicTestingSetup(chainName)
{
    m_wallet_loader = MakeWalletLoader(*m_node.chain, *Assert(m_node.args));

    std::string sep;
    sep += fs::path::preferred_separator;

    m_datadir = gArgs.GetDataDirNet();
    m_cwd = fs::current_path();

    m_walletdir_path_cases["default"] = m_datadir / "wallets";
    m_walletdir_path_cases["custom"] = m_datadir / "my_wallets";
    m_walletdir_path_cases["nonexistent"] = m_datadir / "path_does_not_exist";
    m_walletdir_path_cases["file"] = m_datadir / "not_a_directory.dat";
    m_walletdir_path_cases["trailing"] = m_datadir / ("wallets" + sep);
    m_walletdir_path_cases["trailing2"] = m_datadir / ("wallets" + sep + sep);

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
    gArgs.LockSettings([&](util::Settings& settings) {
        settings.forced_settings.erase("walletdir");
    });
    fs::current_path(m_cwd);
}

void InitWalletDirTestingSetup::SetWalletDir(const fs::path& walletdir_path)
{
    gArgs.ForceSetArg("-walletdir", fs::PathToString(walletdir_path));
}
} // namespace wallet
