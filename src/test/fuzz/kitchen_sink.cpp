// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#include <merkleblock.h>
#include <policy/fees.h>
#include <rpc/util.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/error.h>
#include <util/translation.h>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace {
constexpr TransactionError ALL_TRANSACTION_ERROR[] = {
    TransactionError::OK,
    TransactionError::MISSING_INPUTS,
    TransactionError::ALREADY_IN_CHAIN,
    TransactionError::P2P_DISABLED,
    TransactionError::MEMPOOL_REJECTED,
    TransactionError::MEMPOOL_ERROR,
    TransactionError::INVALID_PST,
    TransactionError::PST_MISMATCH,
    TransactionError::SIGHASH_MISMATCH,
    TransactionError::MAX_FEE_EXCEEDED,
};
}; // namespace

// The fuzzing kitchen sink: Fuzzing harness for functions that need to be
// fuzzed but a.) don't belong in any existing fuzzing harness file, and
// b.) are not important enough to warrant their own fuzzing harness file.
FUZZ_TARGET(kitchen_sink)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const TransactionError transaction_error = fuzzed_data_provider.PickValueInArray(ALL_TRANSACTION_ERROR);
    (void)JSONRPCTransactionError(transaction_error);
    (void)RPCErrorFromTransactionError(transaction_error);
    (void)TransactionErrorString(transaction_error);

    (void)StringForFeeEstimateHorizon(fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS));

    const OutputType output_type = fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES);
    const std::string& output_type_string = FormatOutputType(output_type);
    const std::optional<OutputType> parsed = ParseOutputType(output_type_string);
    assert(parsed);
    assert(output_type == parsed.value());
    (void)ParseOutputType(fuzzed_data_provider.ConsumeRandomLengthString(64));

    const std::vector<uint8_t> bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const std::vector<bool> bits = BytesToBits(bytes);
    const std::vector<uint8_t> bytes_decoded = BitsToBytes(bits);
    assert(bytes == bytes_decoded);
}
