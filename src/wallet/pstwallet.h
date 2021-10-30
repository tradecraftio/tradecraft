// Copyright (c) 2009-2019 The Bitcoin Core developers
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

#ifndef FREICOIN_WALLET_PSTWALLET_H
#define FREICOIN_WALLET_PSTWALLET_H

#include <node/transaction.h>
#include <pst.h>
#include <primitives/transaction.h>
#include <wallet/wallet.h>

/**
 * Fills out a PST with information from the wallet. Fills in UTXOs if we have
 * them. Tries to sign if sign=true. Sets `complete` if the PST is now complete
 * (i.e. has all required signatures or signature-parts, and is ready to
 * finalize.) Sets `error` and returns false if something goes wrong.
 *
 * @param[in]  pwallet pointer to a wallet
 * @param[in]  &pstx reference to PartiallySignedTransaction to fill in
 * @param[out] &complete indicates whether the PST is now complete
 * @param[in]  sighash_type the sighash type to use when signing (if PST does not specify)
 * @param[in]  sign whether to sign or not
 * @param[in]  bip32derivs whether to fill in bip32 derivation information if available
 * return error
 */
NODISCARD TransactionError FillPST(const CWallet* pwallet,
              PartiallySignedTransaction& pstx,
              bool& complete,
              int sighash_type = 1 /* SIGHASH_ALL */,
              bool sign = true,
              bool bip32derivs = false);

#endif // FREICOIN_WALLET_PSTWALLET_H
