// Copyright (c) 2017-2020 The Bitcoin Core developers
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

#ifndef FREICOIN_WALLET_FEEBUMPER_H
#define FREICOIN_WALLET_FEEBUMPER_H

#include <primitives/transaction.h>

class uint256;
enum class FeeEstimateMode;
struct bilingual_str;

namespace wallet {
class CCoinControl;
class CWallet;
class CWalletTx;

namespace feebumper {

enum class Result
{
    OK,
    INVALID_ADDRESS_OR_KEY,
    INVALID_REQUEST,
    INVALID_PARAMETER,
    WALLET_ERROR,
    MISC_ERROR,
};

//! Return whether transaction can be bumped.
bool TransactionCanBeBumped(const CWallet& wallet, const uint256& txid);

//! Create bumpfee transaction based on feerate estimates.
Result CreateRateBumpTransaction(CWallet& wallet,
    const uint256& txid,
    const CCoinControl& coin_control,
    std::vector<bilingual_str>& errors,
    CAmount& old_fee,
    CAmount& new_fee,
    CMutableTransaction& mtx);

//! Sign the new transaction,
//! @return false if the tx couldn't be found or if it was
//! impossible to create the signature(s)
bool SignTransaction(CWallet& wallet, CMutableTransaction& mtx);

//! Commit the bumpfee transaction.
//! @return success in case of CWallet::CommitTransaction was successful,
//! but sets errors if the tx could not be added to the mempool (will try later)
//! or if the old transaction could not be marked as replaced.
Result CommitTransaction(CWallet& wallet,
    const uint256& txid,
    CMutableTransaction&& mtx,
    std::vector<bilingual_str>& errors,
    uint256& bumped_txid);

} // namespace feebumper
} // namespace wallet

#endif // FREICOIN_WALLET_FEEBUMPER_H
