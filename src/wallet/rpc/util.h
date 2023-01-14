// Copyright (c) 2017-2021 The Bitcoin Core developers
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

#ifndef FREICOIN_WALLET_RPC_UTIL_H
#define FREICOIN_WALLET_RPC_UTIL_H

#include <script/script.h>

#include <any>
#include <memory>
#include <string>
#include <vector>

class JSONRPCRequest;
class UniValue;
struct bilingual_str;

namespace wallet {
class CWallet;
class LegacyScriptPubKeyMan;
enum class DatabaseStatus;
struct WalletContext;

extern const std::string HELP_REQUIRING_PASSPHRASE;

/**
 * Figures out what wallet, if any, to use for a JSONRPCRequest.
 *
 * @param[in] request JSONRPCRequest that wishes to access a wallet
 * @return nullptr if no wallet should be used, or a pointer to the CWallet
 */
std::shared_ptr<CWallet> GetWalletForJSONRPCRequest(const JSONRPCRequest& request);
bool GetWalletNameFromJSONRPCRequest(const JSONRPCRequest& request, std::string& wallet_name);

void EnsureWalletIsUnlocked(const CWallet&);
WalletContext& EnsureWalletContext(const std::any& context);
LegacyScriptPubKeyMan& EnsureLegacyScriptPubKeyMan(CWallet& wallet, bool also_create = false);
const LegacyScriptPubKeyMan& EnsureConstLegacyScriptPubKeyMan(const CWallet& wallet);

bool GetAvoidReuseFlag(const CWallet& wallet, const UniValue& param);
bool ParseIncludeWatchonly(const UniValue& include_watchonly, const CWallet& wallet);
std::string LabelFromValue(const UniValue& value);
//! Fetch parent descriptors of this scriptPubKey.
void PushParentDescriptors(const CWallet& wallet, const CScript& script_pubkey, UniValue& entry);

void HandleWalletError(const std::shared_ptr<CWallet> wallet, DatabaseStatus& status, bilingual_str& error);
} //  namespace wallet

#endif // FREICOIN_WALLET_RPC_UTIL_H
