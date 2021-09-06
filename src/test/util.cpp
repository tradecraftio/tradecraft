// Copyright (c) 2019 The Bitcoin Core developers
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

#include <test/util.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <key_io.h>
#include <miner.h>
#include <outputtype.h>
#include <pow.h>
#include <script/standard.h>
#include <validation.h>
#include <validationinterface.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif

const std::string ADDRESS_FCRT1_UNSPENDABLE = "fcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq0nr988";

#ifdef ENABLE_WALLET
std::string getnewaddress(CWallet& w)
{
    constexpr auto output_type = OutputType::BECH32;
    CTxDestination dest;
    std::string error;
    if (!w.GetNewDestination(output_type, "", dest, error)) assert(false);

    return EncodeDestination(dest);
}

void importaddress(CWallet& wallet, const std::string& address)
{
    LOCK(wallet.cs_wallet);
    const auto dest = DecodeDestination(address);
    assert(IsValidDestination(dest));
    const auto script = GetScriptForDestination(dest);
    wallet.MarkDirty();
    assert(!wallet.HaveWatchOnly(script));
    if (!wallet.AddWatchOnly(script, 0 /* nCreateTime */)) assert(false);
    wallet.SetAddressBook(dest, /* label */ "", "receive");
}
#endif // ENABLE_WALLET

std::pair<CTxIn, uint32_t> generatetoaddress(const std::string& address)
{
    const auto dest = DecodeDestination(address);
    assert(IsValidDestination(dest));
    const auto coinbase_script = GetScriptForDestination(dest);

    return MineBlock(coinbase_script);
}

std::pair<CTxIn, uint32_t> MineBlock(const CScript& coinbase_scriptPubKey)
{
    auto block = PrepareBlock(coinbase_scriptPubKey);

    while (!CheckProofOfWork(block->GetHash(), block->nBits, 0, Params().GetConsensus())) {
        ++block->nNonce;
        assert(block->nNonce);
    }

    bool processed{ProcessNewBlock(Params(), block, true, nullptr)};
    assert(processed);

    const CTransaction& coinbase = *block->vtx[0];
    uint32_t n = 0;
    for (; n < (uint32_t)coinbase.vout.size(); ++n) {
        if (coinbase.vout[n].scriptPubKey == coinbase_scriptPubKey) {
            break;
        }
    }
    assert(n < (uint32_t)coinbase.vout.size());

    return {CTxIn{coinbase.GetHash(), n}, coinbase.lock_height};
}

std::shared_ptr<CBlock> PrepareBlock(const CScript& coinbase_scriptPubKey)
{
    auto block = std::make_shared<CBlock>(
        BlockAssembler{Params()}
            .CreateNewBlock(coinbase_scriptPubKey)
            ->block);

    LOCK(cs_main);
    block->nTime = ::ChainActive().Tip()->GetMedianTimePast() + 1;
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    return block;
}
