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

#include <test/util/mining.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <key_io.h>
#include <node/context.h>
#include <pow.h>
#include <script/standard.h>
#include <test/util/script.h>
#include <util/check.h>
#include <validation.h>
#include <versionbits.h>

using node::BlockAssembler;
using node::NodeContext;

CTxIn generatetoaddress(const NodeContext& node, const std::string& address)
{
    const auto dest = DecodeDestination(address);
    assert(IsValidDestination(dest));
    const auto coinbase_script = GetScriptForDestination(dest);

    return MineBlock(node, coinbase_script);
}

std::vector<std::shared_ptr<CBlock>> CreateBlockChain(size_t total_height, const CChainParams& params)
{
    std::vector<std::shared_ptr<CBlock>> ret{total_height};
    auto time{params.GenesisBlock().nTime};
    for (size_t height{0}; height < total_height; ++height) {
        CBlock& block{*(ret.at(height) = std::make_shared<CBlock>())};

        CMutableTransaction coinbase_tx;
        coinbase_tx.vin.resize(1);
        coinbase_tx.vin[0].prevout.SetNull();
        coinbase_tx.vout.resize(1);
        coinbase_tx.vout[0].scriptPubKey = P2WSH_OP_TRUE;
        coinbase_tx.vout[0].nValue = GetBlockSubsidy(height + 1, params.GetConsensus());
        coinbase_tx.vin[0].scriptSig = CScript() << (height + 1) << OP_0;
        block.vtx = {MakeTransactionRef(std::move(coinbase_tx))};

        block.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
        block.hashPrevBlock = (height >= 1 ? *ret.at(height - 1) : params.GenesisBlock()).GetHash();
        block.hashMerkleRoot = BlockMerkleRoot(block);
        block.nTime = ++time;
        block.nBits = params.GenesisBlock().nBits;
        block.nNonce = 0;

        while (!CheckProofOfWork(block.GetHash(), block.nBits, 0, params.GetConsensus())) {
            ++block.nNonce;
            assert(block.nNonce);
        }
    }
    return ret;
}

CTxIn MineBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    auto block = PrepareBlock(node, coinbase_scriptPubKey);

    while (!CheckProofOfWork(block->GetHash(), block->nBits, 0, Params().GetConsensus())) {
        ++block->nNonce;
        assert(block->nNonce);
    }

    bool processed{Assert(node.chainman)->ProcessNewBlock(block, true, true, nullptr)};
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

std::shared_ptr<CBlock> PrepareBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey,
                                     const BlockAssembler::Options& assembler_options)
{
    auto block = std::make_shared<CBlock>(
        BlockAssembler{Assert(node.chainman)->ActiveChainstate(), Assert(node.mempool.get()), assembler_options}
            .CreateNewBlock(coinbase_scriptPubKey)
            ->block);

    LOCK(cs_main);
    block->nTime = Assert(node.chainman)->ActiveChain().Tip()->GetMedianTimePast() + 1;
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    return block;
}
std::shared_ptr<CBlock> PrepareBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    BlockAssembler::Options assembler_options;
    ApplyArgsManOptions(*node.args, assembler_options);
    return PrepareBlock(node, coinbase_scriptPubKey, assembler_options);
}
