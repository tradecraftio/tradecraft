// Copyright (c) 2017 The Bitcoin Core developers
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

#ifndef BITCOIN_WALLET_FEEBUMPER_H
#define BITCOIN_WALLET_FEEBUMPER_H

#include <primitives/transaction.h>

class CWallet;
class CWalletTx;
class uint256;
class CCoinControl;
enum class FeeEstimateMode;

enum class BumpFeeResult
{
    OK,
    INVALID_ADDRESS_OR_KEY,
    INVALID_REQUEST,
    INVALID_PARAMETER,
    WALLET_ERROR,
    MISC_ERROR,
};

class CFeeBumper
{
public:
    CFeeBumper(const CWallet *pWalletIn, const uint256 txidIn, const CCoinControl& coin_control, CAmount totalFee);
    BumpFeeResult getResult() const { return currentResult; }
    const std::vector<std::string>& getErrors() const { return vErrors; }
    CAmount getOldFee() const { return nOldFee; }
    CAmount getNewFee() const { return nNewFee; }
    uint256 getBumpedTxId() const { return bumpedTxid; }

    /* signs the new transaction,
     * returns false if the tx couldn't be found or if it was
     * impossible to create the signature(s)
     */
    bool signTransaction(CWallet *pWallet);

    /* commits the fee bump,
     * returns true, in case of CWallet::CommitTransaction was successful
     * but, eventually sets vErrors if the tx could not be added to the mempool (will try later)
     * or if the old transaction could not be marked as replaced
     */
    bool commit(CWallet *pWalletNonConst);

private:
    bool preconditionChecks(const CWallet *pWallet, const CWalletTx& wtx);

    const uint256 txid;
    uint256 bumpedTxid;
    CMutableTransaction mtx;
    std::vector<std::string> vErrors;
    BumpFeeResult currentResult;
    CAmount nOldFee;
    CAmount nNewFee;
};

#endif // BITCOIN_WALLET_FEEBUMPER_H
