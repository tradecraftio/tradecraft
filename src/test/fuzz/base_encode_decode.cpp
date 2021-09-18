// Copyright (c) 2019-2021 The Bitcoin Core developers
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
#include <pst.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void initialize_base_encode_decode()
{
    static const ECCVerifyHandle verify_handle;
}

FUZZ_TARGET_INIT(base_encode_decode, initialize_base_encode_decode)
{
    const std::string random_encoded_string(buffer.begin(), buffer.end());

    std::vector<unsigned char> decoded;
    if (DecodeBase58(random_encoded_string, decoded, 100)) {
        const std::string encoded_string = EncodeBase58(decoded);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    if (DecodeBase58Check(random_encoded_string, decoded, 100)) {
        const std::string encoded_string = EncodeBase58Check(decoded);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    bool pf_invalid;
    std::string decoded_string = DecodeBase32(random_encoded_string, &pf_invalid);
    if (!pf_invalid) {
        const std::string encoded_string = EncodeBase32(decoded_string);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    decoded_string = DecodeBase64(random_encoded_string, &pf_invalid);
    if (!pf_invalid) {
        const std::string encoded_string = EncodeBase64(decoded_string);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    PartiallySignedTransaction pst;
    std::string error;
    (void)DecodeHexPST(pst, random_encoded_string, error);
}
