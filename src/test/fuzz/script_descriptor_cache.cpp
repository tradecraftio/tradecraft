// Copyright (c) 2020-2021 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <pubkey.h>
#include <script/descriptor.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

FUZZ_TARGET(script_descriptor_cache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    DescriptorCache descriptor_cache;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> code = fuzzed_data_provider.ConsumeBytes<uint8_t>(BIP32_EXTKEY_SIZE);
        if (code.size() == BIP32_EXTKEY_SIZE) {
            CExtPubKey xpub;
            xpub.Decode(code.data());
            const uint32_t key_exp_pos = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            CExtPubKey xpub_fetched;
            if (fuzzed_data_provider.ConsumeBool()) {
                (void)descriptor_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched);
                descriptor_cache.CacheParentExtPubKey(key_exp_pos, xpub);
                assert(descriptor_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched));
            } else {
                const uint32_t der_index = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
                (void)descriptor_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched);
                descriptor_cache.CacheDerivedExtPubKey(key_exp_pos, der_index, xpub);
                assert(descriptor_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched));
            }
            assert(xpub == xpub_fetched);
        }
        (void)descriptor_cache.GetCachedParentExtPubKeys();
        (void)descriptor_cache.GetCachedDerivedExtPubKeys();
    }
}
