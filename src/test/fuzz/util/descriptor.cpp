// Copyright (c) 2023-present The Bitcoin Core developers
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

#include <test/fuzz/util/descriptor.h>

void MockedDescriptorConverter::Init() {
    // The data to use as a private key or a seed for an xprv.
    std::array<std::byte, 32> key_data{std::byte{1}};
    // Generate keys of all kinds and store them in the keys array.
    for (size_t i{0}; i < TOTAL_KEYS_GENERATED; i++) {
        key_data[31] = std::byte(i);

        // If this is a "raw" key, generate a normal privkey. Otherwise generate
        // an extended one.
        if (IdIsCompPubKey(i) || IdIsUnCompPubKey(i) || IdIsXOnlyPubKey(i) || IdIsConstPrivKey(i)) {
            CKey privkey;
            privkey.Set(UCharCast(key_data.begin()), UCharCast(key_data.end()), !IdIsUnCompPubKey(i));
            if (IdIsCompPubKey(i) || IdIsUnCompPubKey(i)) {
                CPubKey pubkey{privkey.GetPubKey()};
                keys_str[i] = HexStr(pubkey);
            } else if (IdIsXOnlyPubKey(i)) {
                const XOnlyPubKey pubkey{privkey.GetPubKey()};
                keys_str[i] = HexStr(pubkey);
            } else {
                keys_str[i] = EncodeSecret(privkey);
            }
        } else {
            CExtKey ext_privkey;
            ext_privkey.SetSeed(key_data);
            if (IdIsXprv(i)) {
                keys_str[i] = EncodeExtKey(ext_privkey);
            } else {
                const CExtPubKey ext_pubkey{ext_privkey.Neuter()};
                keys_str[i] = EncodeExtPubKey(ext_pubkey);
            }
        }
    }
}

std::optional<uint8_t> MockedDescriptorConverter::IdxFromHex(std::string_view hex_characters) const {
    if (hex_characters.size() != 2) return {};
    auto idx = ParseHex(hex_characters);
    if (idx.size() != 1) return {};
    return idx[0];
}

std::optional<std::string> MockedDescriptorConverter::GetDescriptor(std::string_view mocked_desc) const {
    // The smallest fragment would be "pk(%00)"
    if (mocked_desc.size() < 7) return {};

    // The actual descriptor string to be returned.
    std::string desc;
    desc.reserve(mocked_desc.size());

    // Replace all occurrences of '%' followed by two hex characters with the corresponding key.
    for (size_t i = 0; i < mocked_desc.size();) {
        if (mocked_desc[i] == '%') {
            if (i + 3 >= mocked_desc.size()) return {};
            if (const auto idx = IdxFromHex(mocked_desc.substr(i + 1, 2))) {
                desc += keys_str[*idx];
                i += 3;
            } else {
                return {};
            }
        } else {
            desc += mocked_desc[i++];
        }
    }

    return desc;
}

bool HasDeepDerivPath(const FuzzBufferType& buff, const int max_depth)
{
    auto depth{0};
    for (const auto& ch: buff) {
        if (ch == ',') {
            // A comma is always present between two key expressions, so we use that as a delimiter.
            depth = 0;
        } else if (ch == '/') {
            if (++depth > max_depth) return true;
        }
    }
    return false;
}
