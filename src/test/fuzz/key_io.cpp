// Copyright (c) 2020 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <chainparams.h>
#include <key_io.h>
#include <rpc/util.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void initialize()
{
    static const ECCVerifyHandle verify_handle;
    ECC_Start();
    SelectParams(CBaseChainParams::MAIN);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string random_string(buffer.begin(), buffer.end());

    const CKey key = DecodeSecret(random_string);
    if (key.IsValid()) {
        assert(key == DecodeSecret(EncodeSecret(key)));
    }

    const CExtKey ext_key = DecodeExtKey(random_string);
    if (ext_key.key.size() == 32) {
        assert(ext_key == DecodeExtKey(EncodeExtKey(ext_key)));
    }

    const CExtPubKey ext_pub_key = DecodeExtPubKey(random_string);
    if (ext_pub_key.pubkey.size() == CPubKey::COMPRESSED_SIZE) {
        assert(ext_pub_key == DecodeExtPubKey(EncodeExtPubKey(ext_pub_key)));
    }

    const CTxDestination tx_destination = DecodeDestination(random_string);
    (void)DescribeAddress(tx_destination);
    (void)GetKeyForDestination(/* store */ {}, tx_destination);
    (void)GetScriptForDestination(tx_destination);
    (void)IsValidDestination(tx_destination);

    (void)IsValidDestinationString(random_string);
}
