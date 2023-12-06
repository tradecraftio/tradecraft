// Copyright (c) 2019-2022 The Bitcoin Core developers
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

#include <test/fuzz/fuzz.h>

#include <base58.h>
#include <psbt.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(base_encode_decode)
{
    const std::string random_encoded_string(buffer.begin(), buffer.end());

    std::vector<unsigned char> decoded;
    if (DecodeBase58(random_encoded_string, decoded, 100)) {
        const std::string encoded_string = EncodeBase58(decoded);
        assert(encoded_string == TrimStringView(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    if (DecodeBase58Check(random_encoded_string, decoded, 100)) {
        const std::string encoded_string = EncodeBase58Check(decoded);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    auto result = DecodeBase32(random_encoded_string);
    if (result) {
        const std::string encoded_string = EncodeBase32(*result);
        assert(encoded_string == TrimStringView(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    result = DecodeBase64(random_encoded_string);
    if (result) {
        const std::string encoded_string = EncodeBase64(*result);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    PartiallySignedTransaction psbt;
    std::string error;
    (void)DecodeBase64PSBT(psbt, random_encoded_string, error);
}
