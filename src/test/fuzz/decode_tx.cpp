// Copyright (c) 2019-2020 The Bitcoin Core developers
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

#include <core_io.h>
#include <primitives/transaction.h>
#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(decode_tx)
{
    const std::string tx_hex = HexStr(buffer);
    CMutableTransaction mtx;
    const bool result_none = DecodeHexTx(mtx, tx_hex, false, false);
    const bool result_try_witness = DecodeHexTx(mtx, tx_hex, false, true);
    const bool result_try_witness_and_maybe_no_witness = DecodeHexTx(mtx, tx_hex, true, true);
    CMutableTransaction no_witness_mtx;
    const bool result_try_no_witness = DecodeHexTx(no_witness_mtx, tx_hex, true, false);
    assert(!result_none);
    if (result_try_witness_and_maybe_no_witness) {
        assert(result_try_no_witness || result_try_witness);
    }
    if (result_try_no_witness) {
        assert(!no_witness_mtx.HasWitness());
        assert(result_try_witness_and_maybe_no_witness);
    }
}
