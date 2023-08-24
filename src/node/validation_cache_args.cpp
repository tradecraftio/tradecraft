// Copyright (c) 2022 The Bitcoin Core developers
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

#include <node/validation_cache_args.h>

#include <kernel/validation_cache_sizes.h>

#include <util/system.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

using kernel::ValidationCacheSizes;

namespace node {
void ApplyArgsManOptions(const ArgsManager& argsman, ValidationCacheSizes& cache_sizes)
{
    if (auto max_size = argsman.GetIntArg("-maxsigcachesize")) {
        // 1. When supplied with a max_size of 0, both InitSignatureCache and
        //    InitScriptExecutionCache create the minimum possible cache (2
        //    elements). Therefore, we can use 0 as a floor here.
        // 2. Multiply first, divide after to avoid integer truncation.
        size_t clamped_size_each = std::max<int64_t>(*max_size, 0) * (1 << 20) / 2;
        cache_sizes = {
            .signature_cache_bytes = clamped_size_each,
            .script_execution_cache_bytes = clamped_size_each,
        };
    }
}
} // namespace node
