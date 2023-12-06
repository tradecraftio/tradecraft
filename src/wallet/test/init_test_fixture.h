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

#ifndef FREICOIN_WALLET_TEST_INIT_TEST_FIXTURE_H
#define FREICOIN_WALLET_TEST_INIT_TEST_FIXTURE_H

#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <node/context.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>


namespace wallet {
struct InitWalletDirTestingSetup: public BasicTestingSetup {
    explicit InitWalletDirTestingSetup(const ChainType chain_type = ChainType::MAIN);
    ~InitWalletDirTestingSetup();
    void SetWalletDir(const fs::path& walletdir_path);

    fs::path m_datadir;
    fs::path m_cwd;
    std::map<std::string, fs::path> m_walletdir_path_cases;
    std::unique_ptr<interfaces::WalletLoader> m_wallet_loader;
};

#endif // FREICOIN_WALLET_TEST_INIT_TEST_FIXTURE_H
} // namespace wallet
