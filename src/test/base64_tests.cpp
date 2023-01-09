// Copyright (c) 2011-2021 The Bitcoin Core developers
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

#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>
#include <string>

using namespace std::literals;

BOOST_AUTO_TEST_SUITE(base64_tests)

BOOST_AUTO_TEST_CASE(base64_testvectors)
{
    static const std::string vstrIn[]  = {"","f","fo","foo","foob","fooba","foobar"};
    static const std::string vstrOut[] = {"","Zg==","Zm8=","Zm9v","Zm9vYg==","Zm9vYmE=","Zm9vYmFy"};
    for (unsigned int i=0; i<std::size(vstrIn); i++)
    {
        std::string strEnc = EncodeBase64(vstrIn[i]);
        BOOST_CHECK_EQUAL(strEnc, vstrOut[i]);
        std::string strDec = DecodeBase64(strEnc);
        BOOST_CHECK_EQUAL(strDec, vstrIn[i]);
    }

    {
        const std::vector<uint8_t> in_u{0xff, 0x01, 0xff};
        const std::vector<std::byte> in_b{std::byte{0xff}, std::byte{0x01}, std::byte{0xff}};
        const std::string in_s{"\xff\x01\xff"};
        const std::string out_exp{"/wH/"};
        BOOST_CHECK_EQUAL(EncodeBase64(in_u), out_exp);
        BOOST_CHECK_EQUAL(EncodeBase64(in_b), out_exp);
        BOOST_CHECK_EQUAL(EncodeBase64(in_s), out_exp);
    }

    // Decoding strings with embedded NUL characters should fail
    bool failure;
    (void)DecodeBase64("invalid\0"s, &failure);
    BOOST_CHECK(failure);
    (void)DecodeBase64("nQB/pZw="s, &failure);
    BOOST_CHECK(!failure);
    (void)DecodeBase64("nQB/pZw=\0invalid"s, &failure);
    BOOST_CHECK(failure);
    (void)DecodeBase64("nQB/pZw=invalid\0"s, &failure);
    BOOST_CHECK(failure);
}

BOOST_AUTO_TEST_SUITE_END()
