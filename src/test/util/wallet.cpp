// Copyright (c) 2019 The Bitcoin Core developers
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

#include <test/util/wallet.h>

#include <key_io.h>
#include <outputtype.h>
#include <script/standard.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif

const std::string ADDRESS_BCRT1_UNSPENDABLE = "bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq3xueyj";

#ifdef ENABLE_WALLET
std::string getnewaddress(CWallet& w)
{
    constexpr auto output_type = OutputType::BECH32;
    CTxDestination dest;
    std::string error;
    if (!w.GetNewDestination(output_type, "", dest, error)) assert(false);

    return EncodeDestination(dest);
}

void importaddress(CWallet& wallet, const std::string& address)
{
    auto spk_man = wallet.GetLegacyScriptPubKeyMan();
    LOCK2(wallet.cs_wallet, spk_man->cs_KeyStore);
    const auto dest = DecodeDestination(address);
    assert(IsValidDestination(dest));
    const auto script = GetScriptForDestination(dest);
    wallet.MarkDirty();
    assert(!spk_man->HaveWatchOnly(script));
    if (!spk_man->AddWatchOnly(script, 0 /* nCreateTime */)) assert(false);
    wallet.SetAddressBook(dest, /* label */ "", "receive");
}
#endif // ENABLE_WALLET
