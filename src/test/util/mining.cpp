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

#include <test/util/mining.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <key_io.h>
#include <miner.h>
#include <node/context.h>
#include <pow.h>
#include <script/standard.h>
#include <validation.h>

CTxIn generatetoaddress(const NodeContext& node, const std::string& address)
{
    const auto dest = DecodeDestination(address);
    assert(IsValidDestination(dest));
    const auto coinbase_script = GetScriptForDestination(dest);

    return MineBlock(node, coinbase_script);
}

CTxIn MineBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    auto block = PrepareBlock(node, coinbase_scriptPubKey);

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

    return CTxIn{coinbase.GetHash(), n};
}

std::shared_ptr<CBlock> PrepareBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    assert(node.mempool);
    auto block = std::make_shared<CBlock>(
        BlockAssembler{*node.mempool, Params()}
            .CreateNewBlock(coinbase_scriptPubKey)
            ->block);

    LOCK(cs_main);
    block->nTime = ::ChainActive().Tip()->GetMedianTimePast() + 1;
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    return block;
}
