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

#ifndef FREICOIN_UTIL_RBF_H
#define FREICOIN_UTIL_RBF_H

#include <cstdint>

class CTransaction;

static constexpr uint32_t MAX_BIP125_RBF_SEQUENCE{0xfffffffd};

/** Check whether the sequence numbers on this transaction are signaling opt-in to replace-by-fee,
 * according to BIP 125.  Allow opt-out of transaction replacement by setting nSequence >
 * MAX_BIP125_RBF_SEQUENCE (SEQUENCE_FINAL-2) on all inputs.
*
* SEQUENCE_FINAL-1 is picked to still allow use of nLockTime by non-replaceable transactions. All
* inputs rather than just one is for the sake of multi-party protocols, where we don't want a single
* party to be able to disable replacement by opting out in their own input. */
bool SignalsOptInRBF(const CTransaction& tx);

#endif // FREICOIN_UTIL_RBF_H
