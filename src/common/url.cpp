// Copyright (c) 2015-2022 The Bitcoin Core developers
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

#include <common/url.h>

#include <charconv>
#include <string>
#include <string_view>
#include <system_error>

std::string UrlDecode(std::string_view url_encoded)
{
    std::string res;
    res.reserve(url_encoded.size());

    for (size_t i = 0; i < url_encoded.size(); ++i) {
        char c = url_encoded[i];
        // Special handling for percent which should be followed by two hex digits
        // representing an octet values, see RFC 3986, Section 2.1 Percent-Encoding
        if (c == '%' && i + 2 < url_encoded.size()) {
            unsigned int decoded_value{0};
            auto [p, ec] = std::from_chars(url_encoded.data() + i + 1, url_encoded.data() + i + 3, decoded_value, 16);

            // Only if there is no error and the pointer is set to the end of
            // the string, we can be sure both characters were valid hex
            if (ec == std::errc{} && p == url_encoded.data() + i + 3) {
                res += static_cast<char>(decoded_value);
                // Next two characters are part of the percent encoding
                i += 2;
                continue;
            }
            // In case of invalid percent encoding, add the '%' and continue
        }
        res += c;
    }

    return res;
}
