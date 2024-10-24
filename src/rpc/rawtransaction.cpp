// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#include <base58.h>
#include <chain.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <index/txindex.h>
#include <key_io.h>
#include <node/blockstorage.h>
#include <node/coin.h>
#include <node/context.h>
#include <node/pst.h>
#include <node/transaction.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <pst.h>
#include <random.h>
#include <rpc/blockchain.h>
#include <rpc/rawtransaction_util.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <uint256.h>
#include <undo.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/vector.h>
#include <validation.h>
#include <validationinterface.h>

#include <numeric>
#include <stdint.h>

#include <univalue.h>

using node::AnalyzePST;
using node::FindCoins;
using node::GetTransaction;
using node::NodeContext;
using node::PSTAnalysis;

static void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry,
                     Chainstate& active_chainstate, const CTxUndo* txundo = nullptr,
                     TxVerbosity verbosity = TxVerbosity::SHOW_DETAILS)
{
    CHECK_NONFATAL(verbosity >= TxVerbosity::SHOW_DETAILS);
    // Call into TxToUniv() in freicoin-common to decode the transaction hex.
    //
    // Blockchain contextual information (confirmations and blocktime) is not
    // available to code in freicoin-common, so we query them here and push the
    // data into the returned UniValue.
    TxToUniv(tx, /*block_hash=*/uint256(), entry, /*include_hex=*/true, txundo, verbosity);

    if (!hashBlock.IsNull()) {
        LOCK(cs_main);

        entry.pushKV("blockhash", hashBlock.GetHex());
        const CBlockIndex* pindex = active_chainstate.m_blockman.LookupBlockIndex(hashBlock);
        if (pindex) {
            if (active_chainstate.m_chain.Contains(pindex)) {
                entry.pushKV("confirmations", 1 + active_chainstate.m_chain.Height() - pindex->nHeight);
                entry.pushKV("time", pindex->GetBlockTime());
                entry.pushKV("blocktime", pindex->GetBlockTime());
            }
            else
                entry.pushKV("confirmations", 0);
        }
    }
}

static std::vector<RPCResult> ScriptPubKeyDoc() {
    return
         {
             {RPCResult::Type::STR, "asm", "Disassembly of the public key script"},
             {RPCResult::Type::STR, "desc", "Inferred descriptor for the output"},
             {RPCResult::Type::STR_HEX, "hex", "The raw public key script bytes, hex-encoded"},
             {RPCResult::Type::STR, "address", /*optional=*/true, "The Freicoin address (only if a well-defined address exists)"},
             {RPCResult::Type::STR, "type", "The type (one of: " + GetAllOutputTypes() + ")"},
         };
}

static std::vector<RPCResult> DecodeTxDoc(const std::string& txid_field_doc)
{
    return {
        {RPCResult::Type::STR_HEX, "txid", txid_field_doc},
        {RPCResult::Type::STR_HEX, "hash", "The transaction hash (differs from txid for witness transactions)"},
        {RPCResult::Type::NUM, "size", "The serialized transaction size"},
        {RPCResult::Type::NUM, "vsize", "The virtual transaction size (differs from size for witness transactions)"},
        {RPCResult::Type::NUM, "weight", "The transaction's weight (between vsize*4-3 and vsize*4)"},
        {RPCResult::Type::NUM, "version", "The version"},
        {RPCResult::Type::NUM_TIME, "locktime", "The lock time"},
        {RPCResult::Type::NUM, "lockheight", "The reference height, and the minimum height for inclusion in chain."},
        {RPCResult::Type::ARR, "vin", "",
        {
            {RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "coinbase", /*optional=*/true, "The coinbase value (only if coinbase transaction)"},
                {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id (if not coinbase transaction)"},
                {RPCResult::Type::NUM, "vout", /*optional=*/true, "The output number (if not coinbase transaction)"},
                {RPCResult::Type::OBJ, "scriptSig", /*optional=*/true, "The script (if not coinbase transaction)",
                {
                    {RPCResult::Type::STR, "asm", "Disassembly of the signature script"},
                    {RPCResult::Type::STR_HEX, "hex", "The raw signature script bytes, hex-encoded"},
                }},
                {RPCResult::Type::ARR, "txinwitness", /*optional=*/true, "",
                {
                    {RPCResult::Type::STR_HEX, "hex", "hex-encoded witness data (if any)"},
                }},
                {RPCResult::Type::NUM, "sequence", "The script sequence number"},
            }},
        }},
        {RPCResult::Type::ARR, "vout", "",
        {
            {RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_AMOUNT, "value", "The value in " + CURRENCY_UNIT},
                {RPCResult::Type::NUM, "n", "index"},
                {RPCResult::Type::OBJ, "scriptPubKey", "", ScriptPubKeyDoc()},
            }},
        }},
    };
}

static std::vector<RPCArg> CreateTxDoc()
{
    return {
        {"inputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The inputs",
            {
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                    {
                        {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                        {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                        {"sequence", RPCArg::Type::NUM, RPCArg::DefaultHint{"depends on the value of the 'replaceable' and 'locktime' arguments"}, "The sequence number"},
                    },
                },
            },
        },
        {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The outputs specified as key-value pairs.\n"
                "Each key may only appear once, i.e. there can only be one 'data' output, and no address may be duplicated.\n"
                "At least one output of either type must be specified.\n"
                "For compatibility reasons, a dictionary, which holds the key-value pairs directly, is also\n"
                "                             accepted as second parameter.",
            {
                {"", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::OMITTED, "",
                    {
                        {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "A key-value pair. The key (string) is the freicoin address, the value (float or string) is the amount in " + CURRENCY_UNIT},
                    },
                },
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                    {
                        {"data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A key-value pair. The key must be \"data\", the value is hex-encoded data"},
                    },
                },
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                    {
                        {"destroy", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "An amount of freicoin to be destroyed (sent to an OP_RETURN output)."},
                    },
                },
            },
         RPCArgOptions{.skip_type_check = true}},
        {"locktime", RPCArg::Type::NUM, RPCArg::Default{0}, "Raw locktime. Non-0 value also locktime-activates inputs"},
        {"lockheight", RPCArg::Type::NUM, RPCArg::Default{0}, "The reference height of the outputs in the transaction being generated, and the minimum height for inclusion in chain. If not specified, the height of the next block to be mined is used."},
        {"replaceable", RPCArg::Type::BOOL, RPCArg::Default{true}, "Marks this transaction as BIP125-replaceable.\n"
                "Allows this transaction to be replaced by a transaction with higher fees. If provided, it is an error if explicit sequence numbers are incompatible."},
    };
}

// Update PST with information from the mempool, the UTXO set, the txindex, and the provided descriptors.
// Optionally, sign the inputs that we can using information from the descriptors.
PartiallySignedTransaction ProcessPST(const std::string& pst_string, const std::any& context, const HidingSigningProvider& provider, int sighash_type, bool finalize)
{
    // Unserialize the transactions
    PartiallySignedTransaction pstx;
    std::string error;
    if (!DecodeHexPST(pstx, pst_string, error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    if (g_txindex) g_txindex->BlockUntilSyncedToCurrentChain();
    const NodeContext& node = EnsureAnyNodeContext(context);

    // If we can't find the corresponding full transaction for all of our inputs,
    // this will be used to find just the utxos for the segwit inputs for which
    // the full transaction isn't found
    std::map<COutPoint, Coin> coins;

    // Fetch previous transactions:
    // First, look in the txindex and the mempool
    for (unsigned int i = 0; i < pstx.tx->vin.size(); ++i) {
        PSTInput& pst_input = pstx.inputs.at(i);
        const CTxIn& tx_in = pstx.tx->vin.at(i);

        // The `non_witness_utxo` is the whole previous transaction
        if (pst_input.non_witness_utxo) continue;

        CTransactionRef tx;

        // Look in the txindex
        if (g_txindex) {
            uint256 block_hash;
            g_txindex->FindTx(tx_in.prevout.hash, block_hash, tx);
        }
        // If we still don't have it look in the mempool
        if (!tx) {
            tx = node.mempool->get(tx_in.prevout.hash);
        }
        if (tx) {
            pst_input.non_witness_utxo = tx;
        } else {
            coins[tx_in.prevout]; // Create empty map entry keyed by prevout
        }
    }

    // If we still haven't found all of the inputs, look for the missing ones in the utxo set
    if (!coins.empty()) {
        FindCoins(node, coins);
        for (unsigned int i = 0; i < pstx.tx->vin.size(); ++i) {
            PSTInput& input = pstx.inputs.at(i);

            // If there are still missing utxos, add them if they were found in the utxo set
            if (!input.non_witness_utxo) {
                const CTxIn& tx_in = pstx.tx->vin.at(i);
                const Coin& coin = coins.at(tx_in.prevout);
                if (!coin.out.IsNull() && IsSegWitOutput(provider, coin.out.scriptPubKey)) {
                    input.witness_utxo = coin.out;
                }
            }
        }
    }

    const PrecomputedTransactionData& txdata = PrecomputePSTData(pstx);

    for (unsigned int i = 0; i < pstx.tx->vin.size(); ++i) {
        if (PSTInputSigned(pstx.inputs.at(i))) {
            continue;
        }

        // Update script/keypath information using descriptor data.
        // Note that SignPSTInput does a lot more than just constructing ECDSA signatures.
        // We only actually care about those if our signing provider doesn't hide private
        // information, as is the case with `descriptorprocesspst`
        SignPSTInput(provider, pstx, /*index=*/i, &txdata, sighash_type, /*out_sigdata=*/nullptr, finalize);
    }

    // Update script/keypath information using descriptor data.
    for (unsigned int i = 0; i < pstx.tx->vout.size(); ++i) {
        UpdatePSTOutput(provider, pstx, i);
    }

    RemoveUnnecessaryTransactions(pstx, /*sighash_type=*/1);

    return pstx;
}

static RPCHelpMan getrawtransaction()
{
    return RPCHelpMan{
                "getrawtransaction",

                "By default, this call only returns a transaction if it is in the mempool. If -txindex is enabled\n"
                "and no blockhash argument is passed, it will return the transaction if it is in the mempool or any block.\n"
                "If a blockhash argument is passed, it will return the transaction if\n"
                "the specified block is available and the transaction is in that block.\n\n"
                "Hint: Use gettransaction for wallet transactions.\n\n"

                "If verbosity is 0 or omitted, returns the serialized transaction as a hex-encoded string.\n"
                "If verbosity is 1, returns a JSON Object with information about the transaction.\n"
                "If verbosity is 2, returns a JSON Object with information about the transaction, including fee and prevout information.",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                    {"verbosity|verbose", RPCArg::Type::NUM, RPCArg::Default{0}, "0 for hex-encoded data, 1 for a JSON object, and 2 for JSON object with fee and prevout",
                     RPCArgOptions{.skip_type_check = true}},
                    {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The block in which to look for the transaction"},
                },
                {
                    RPCResult{"if verbosity is not set or set to 0",
                         RPCResult::Type::STR, "data", "The serialized transaction as a hex-encoded string for 'txid'"
                     },
                     RPCResult{"if verbosity is set to 1",
                         RPCResult::Type::OBJ, "", "",
                         Cat<std::vector<RPCResult>>(
                         {
                             {RPCResult::Type::BOOL, "in_active_chain", /*optional=*/true, "Whether specified block is in the active chain or not (only present with explicit \"blockhash\" argument)"},
                             {RPCResult::Type::STR_HEX, "blockhash", /*optional=*/true, "the block hash"},
                             {RPCResult::Type::NUM, "confirmations", /*optional=*/true, "The confirmations"},
                             {RPCResult::Type::NUM_TIME, "blocktime", /*optional=*/true, "The block time expressed in " + UNIX_EPOCH_TIME},
                             {RPCResult::Type::NUM, "time", /*optional=*/true, "Same as \"blocktime\""},
                             {RPCResult::Type::STR_HEX, "hex", "The serialized, hex-encoded data for 'txid'"},
                         },
                         DecodeTxDoc(/*txid_field_doc=*/"The transaction id (same as provided)")),
                    },
                    RPCResult{"for verbosity = 2",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::ELISION, "", "Same output as verbosity = 1"},
                            {RPCResult::Type::NUM, "fee", /*optional=*/true, "transaction fee in " + CURRENCY_UNIT + ", omitted if block undo data is not available"},
                            {RPCResult::Type::ARR, "vin", "",
                            {
                                {RPCResult::Type::OBJ, "", "utxo being spent",
                                {
                                    {RPCResult::Type::ELISION, "", "Same output as verbosity = 1"},
                                    {RPCResult::Type::OBJ, "prevout", /*optional=*/true, "The previous output, omitted if block undo data is not available",
                                    {
                                        {RPCResult::Type::BOOL, "generated", "Coinbase or not"},
                                        {RPCResult::Type::NUM, "height", "The height of the prevout"},
                                        {RPCResult::Type::STR_AMOUNT, "value", "The value in " + CURRENCY_UNIT},
                                        {RPCResult::Type::OBJ, "scriptPubKey", "", ScriptPubKeyDoc()},
                                    }},
                                }},
                            }},
                        }},
                },
                RPCExamples{
                    HelpExampleCli("getrawtransaction", "\"mytxid\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" 1")
            + HelpExampleRpc("getrawtransaction", "\"mytxid\", 1")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" 0 \"myblockhash\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" 1 \"myblockhash\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" 2 \"myblockhash\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    ChainstateManager& chainman = EnsureChainman(node);

    uint256 hash = ParseHashV(request.params[0], "parameter 1");
    const CBlockIndex* blockindex = nullptr;

    if (hash == chainman.GetParams().GenesisBlock().hashMerkleRoot) {
        // Special exception for the genesis block coinbase transaction
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "The genesis block coinbase is not considered an ordinary transaction and cannot be retrieved");
    }

    // Accept either a bool (true) or a num (>=0) to indicate verbosity.
    int verbosity{0};
    if (!request.params[1].isNull()) {
        if (request.params[1].isBool()) {
            verbosity = request.params[1].get_bool();
        } else {
            verbosity = request.params[1].getInt<int>();
        }
    }

    if (!request.params[2].isNull()) {
        LOCK(cs_main);

        uint256 blockhash = ParseHashV(request.params[2], "parameter 3");
        blockindex = chainman.m_blockman.LookupBlockIndex(blockhash);
        if (!blockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found");
        }
    }

    bool f_txindex_ready = false;
    if (g_txindex && !blockindex) {
        f_txindex_ready = g_txindex->BlockUntilSyncedToCurrentChain();
    }

    uint256 hash_block;
    const CTransactionRef tx = GetTransaction(blockindex, node.mempool.get(), hash, hash_block, chainman.m_blockman);
    if (!tx) {
        std::string errmsg;
        if (blockindex) {
            const bool block_has_data = WITH_LOCK(::cs_main, return blockindex->nStatus & BLOCK_HAVE_DATA);
            if (!block_has_data) {
                throw JSONRPCError(RPC_MISC_ERROR, "Block not available");
            }
            errmsg = "No such transaction found in the provided block";
        } else if (!g_txindex) {
            errmsg = "No such mempool transaction. Use -txindex or provide a block hash to enable blockchain transaction queries";
        } else if (!f_txindex_ready) {
            errmsg = "No such mempool transaction. Blockchain transactions are still in the process of being indexed";
        } else {
            errmsg = "No such mempool or blockchain transaction";
        }
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, errmsg + ". Use gettransaction for wallet transactions.");
    }

    if (verbosity <= 0) {
        return EncodeHexTx(*tx);
    }

    UniValue result(UniValue::VOBJ);
    if (blockindex) {
        LOCK(cs_main);
        result.pushKV("in_active_chain", chainman.ActiveChain().Contains(blockindex));
    }
    // If request is verbosity >= 1 but no blockhash was given, then look up the blockindex
    if (request.params[2].isNull()) {
        LOCK(cs_main);
        blockindex = chainman.m_blockman.LookupBlockIndex(hash_block); // May be nullptr for mempool transactions
    }
    if (verbosity == 1) {
        TxToJSON(*tx, hash_block, result, chainman.ActiveChainstate());
        return result;
    }

    CBlockUndo blockUndo;
    CBlock block;

    if (tx->IsCoinBase() || !blockindex || WITH_LOCK(::cs_main, return chainman.m_blockman.IsBlockPruned(*blockindex)) ||
        !(chainman.m_blockman.UndoReadFromDisk(blockUndo, *blockindex) && chainman.m_blockman.ReadBlockFromDisk(block, *blockindex))) {
        TxToJSON(*tx, hash_block, result, chainman.ActiveChainstate());
        return result;
    }

    CTxUndo* undoTX {nullptr};
    auto it = std::find_if(block.vtx.begin(), block.vtx.end(), [tx](CTransactionRef t){ return *t == *tx; });
    if (it != block.vtx.end()) {
        // -1 as blockundo does not have coinbase tx
        undoTX = &blockUndo.vtxundo.at(it - block.vtx.begin() - 1);
    }
    TxToJSON(*tx, hash_block, result, chainman.ActiveChainstate(), undoTX, TxVerbosity::SHOW_DETAILS_AND_PREVOUT);
    return result;
},
    };
}

static RPCHelpMan createrawtransaction()
{
    return RPCHelpMan{"createrawtransaction",
                "\nCreate a transaction spending the given inputs and creating new outputs.\n"
                "Outputs can be addresses or data.\n"
                "Returns hex-encoded raw transaction.\n"
                "Note that the transaction's inputs are not signed, and\n"
                "it is not stored in the wallet or transmitted to the network.\n",
                CreateTxDoc(),
                RPCResult{
                    RPCResult::Type::STR_HEX, "transaction", "hex string of the transaction"
                },
                RPCExamples{
                    HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"address\\\":0.01}]\"")
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
            + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"[{\\\"address\\\":0.01}]\"")
            + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    int height = -1;
    {
        LOCK(cs_main);
        height = chainman.ActiveChain().Height();
    }

    std::optional<bool> rbf;
    if (!request.params[4].isNull()) {
        rbf = request.params[4].get_bool();
    }
    CMutableTransaction rawTx = ConstructTransaction(request.params[0], request.params[1], request.params[2], request.params[3], height, rbf);

    return EncodeHexTx(CTransaction(rawTx));
},
    };
}

static RPCHelpMan decoderawtransaction()
{
    return RPCHelpMan{"decoderawtransaction",
                "Return a JSON object representing the serialized, hex-encoded transaction.",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction hex string"},
                    {"iswitness", RPCArg::Type::BOOL, RPCArg::DefaultHint{"depends on heuristic tests"}, "Whether the transaction hex is a serialized witness transaction.\n"
                        "If iswitness is not present, heuristic tests will be used in decoding.\n"
                        "If true, only witness deserialization will be tried.\n"
                        "If false, only non-witness deserialization will be tried.\n"
                        "This boolean should reflect whether the transaction has inputs\n"
                        "(e.g. fully valid, or on-chain transactions), if known by the caller."
                    },
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    DecodeTxDoc(/*txid_field_doc=*/"The transaction id"),
                },
                RPCExamples{
                    HelpExampleCli("decoderawtransaction", "\"hexstring\"")
            + HelpExampleRpc("decoderawtransaction", "\"hexstring\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    CMutableTransaction mtx;

    bool try_witness = request.params[1].isNull() ? true : request.params[1].get_bool();
    bool try_no_witness = request.params[1].isNull() ? true : !request.params[1].get_bool();

    if (!DecodeHexTx(mtx, request.params[0].get_str(), try_no_witness, try_witness)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    UniValue result(UniValue::VOBJ);
    TxToUniv(CTransaction(std::move(mtx)), /*block_hash=*/uint256(), /*entry=*/result, /*include_hex=*/false);

    return result;
},
    };
}

static RPCHelpMan decodescript()
{
    return RPCHelpMan{
        "decodescript",
        "\nDecode a hex-encoded script.\n",
        {
            {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hex-encoded script"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "asm", "Script public key"},
                {RPCResult::Type::STR, "desc", "Inferred descriptor for the script"},
                {RPCResult::Type::STR, "type", "The output type (e.g. " + GetAllOutputTypes() + ")"},
                {RPCResult::Type::STR, "address", /*optional=*/true, "The Freicoin address (only if a well-defined address exists)"},
                {RPCResult::Type::STR, "p2sh", /*optional=*/true,
                 "address of P2SH script wrapping this redeem script (not returned for types that should not be wrapped)"},
                {RPCResult::Type::OBJ, "segwit", /*optional=*/true,
                 "Result of a witness script public key wrapping this redeem script (not returned for types that should not be wrapped)",
                 {
                     {RPCResult::Type::STR, "asm", "String representation of the script public key"},
                     {RPCResult::Type::STR_HEX, "hex", "Hex string of the script public key"},
                     {RPCResult::Type::STR, "type", "The type of the script public key (e.g. witness_v0_shorthash or witness_v0_longhash)"},
                     {RPCResult::Type::STR, "address", /*optional=*/true, "The Freicoin address (only if a well-defined address exists)"},
                     {RPCResult::Type::STR, "desc", "Inferred descriptor for the script"},
                 }},
            },
        },
        RPCExamples{
            HelpExampleCli("decodescript", "\"hexstring\"")
          + HelpExampleRpc("decodescript", "\"hexstring\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue r(UniValue::VOBJ);
    CScript script;
    if (request.params[0].get_str().size() > 0){
        std::vector<unsigned char> scriptData(ParseHexV(request.params[0], "argument"));
        script = CScript(scriptData.begin(), scriptData.end());
    } else {
        // Empty scripts are valid
    }
    ScriptToUniv(script, /*out=*/r, /*include_hex=*/false, /*include_address=*/true);

    std::vector<std::vector<unsigned char>> solutions_data;
    const TxoutType which_type{Solver(script, solutions_data)};

    const bool can_wrap{[&] {
        switch (which_type) {
        case TxoutType::MULTISIG:
        case TxoutType::NONSTANDARD:
        case TxoutType::PUBKEY:
        case TxoutType::PUBKEYHASH:
            // Can be wrapped if the checks below pass
            break;
        case TxoutType::NULL_DATA:
        case TxoutType::UNSPENDABLE:
        case TxoutType::SCRIPTHASH:
        case TxoutType::WITNESS_UNKNOWN:
        case TxoutType::WITNESS_V0_SHORTHASH:
        case TxoutType::WITNESS_V0_LONGHASH:
            // Should not be wrapped
            return false;
        } // no default case, so the compiler can warn about missing cases
        if (!script.HasValidOps() || script.IsUnspendable()) {
            return false;
        }
        for (CScript::const_iterator it{script.begin()}; it != script.end();) {
            opcodetype op;
            CHECK_NONFATAL(script.GetOp(it, op));
            if (op == OP_CHECKSIGADD || IsOpSuccess(op)) {
                return false;
            }
        }
        return true;
    }()};

    if (can_wrap) {
        r.pushKV("p2sh", EncodeDestination(ScriptHash(script)));
        // P2SH and witness programs cannot be wrapped in P2WSH, if this script
        // is a witness program, don't return addresses for a segwit programs.
        const bool can_wrap_P2WSH{[&] {
            switch (which_type) {
            case TxoutType::MULTISIG:
            case TxoutType::PUBKEY:
            // Uncompressed pubkeys cannot be used with segwit checksigs.
            // If the script contains an uncompressed pubkey, skip encoding of a segwit program.
                for (const auto& solution : solutions_data) {
                    if ((solution.size() != 1) && !CPubKey(solution).IsCompressed()) {
                        return false;
                    }
                }
                return true;
            case TxoutType::PUBKEYHASH:
            // If the script is P2PKH, we have no way of checking if the
            // corresponding pubkey is compressed or not.  And even if we did,
            // "P2WPKH" scripts aren't recognized by default.
                return false;
            case TxoutType::NONSTANDARD:
                // Can be P2WSH wrapped
                return true;
            case TxoutType::NULL_DATA:
            case TxoutType::UNSPENDABLE:
            case TxoutType::SCRIPTHASH:
            case TxoutType::WITNESS_UNKNOWN:
            case TxoutType::WITNESS_V0_SHORTHASH:
            case TxoutType::WITNESS_V0_LONGHASH:
                // Should not be wrapped
                return false;
            } // no default case, so the compiler can warn about missing cases
            NONFATAL_UNREACHABLE();
        }()};
        if (can_wrap_P2WSH) {
            UniValue sr(UniValue::VOBJ);
            CScript segwitScr;
            FlatSigningProvider provider;
            if (which_type == TxoutType::PUBKEY) {
                segwitScr = GetScriptForDestination(WitnessV0ShortHash(0 /* version */, CPubKey(solutions_data[0])));
            } else {
                // Scripts that are not fit for P2WPK are encoded as P2WSH.
                WitnessV0ScriptEntry entry(0 /* vesion */, script);
                WitnessV0LongHash longid = entry.GetLongHash();
                provider.witscripts[WitnessV0ShortHash(longid)] = entry;
                segwitScr = GetScriptForDestination(longid);
            }
            ScriptToUniv(segwitScr, /*out=*/sr, /*include_hex=*/true, /*include_address=*/true, /*provider=*/&provider);
            r.pushKV("segwit", sr);
        }
    }

    return r;
},
    };
}

static RPCHelpMan combinerawtransaction()
{
    return RPCHelpMan{"combinerawtransaction",
                "\nCombine multiple partially signed transactions into one transaction.\n"
                "The combined transaction may be another partially signed transaction or a \n"
                "fully signed transaction.",
                {
                    {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The hex strings of partially signed transactions",
                        {
                            {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A hex-encoded raw transaction"},
                        },
                        },
                },
                RPCResult{
                    RPCResult::Type::STR, "", "The hex-encoded raw transaction with signature(s)"
                },
                RPCExamples{
                    HelpExampleCli("combinerawtransaction", R"('["myhex1", "myhex2", "myhex3"]')")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    UniValue txs = request.params[0].get_array();
    std::vector<CMutableTransaction> txVariants(txs.size());

    for (unsigned int idx = 0; idx < txs.size(); idx++) {
        if (!DecodeHexTx(txVariants[idx], txs[idx].get_str())) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed for tx %d. Make sure the tx has at least one input.", idx));
        }
    }

    if (txVariants.empty()) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Missing transactions");
    }

    // Merging transactions with different lock_height values is unlikely to
    // accomplish what the user is expecting, since this field also acts as the
    // reference height for the transaction.  We require all transactions to
    // have maching lock_height values.
    auto lock_height = txVariants[0].lock_height;
    for (const auto& tx : txVariants) {
        if (tx.lock_height != lock_height) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Provided transactions have incompatible lock_height fields");
        }
    }

    // mergedTx will end up with all the signatures; it
    // starts as a clone of the rawtx:
    CMutableTransaction mergedTx(txVariants[0]);

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        NodeContext& node = EnsureAnyNodeContext(request.context);
        const CTxMemPool& mempool = EnsureMemPool(node);
        ChainstateManager& chainman = EnsureChainman(node);
        LOCK2(cs_main, mempool.cs);
        CCoinsViewCache &viewChain = chainman.ActiveChainstate().CoinsTip();
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : mergedTx.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mergedTx);
    // Sign what we can:
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++) {
        CTxIn& txin = mergedTx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            throw JSONRPCError(RPC_VERIFY_ERROR, "Input not found or already spent");
        }
        SignatureData sigdata;

        // ... and merge in other signatures:
        for (const CMutableTransaction& txv : txVariants) {
            if (txv.vin.size() > i) {
                sigdata.MergeSignatureData(DataFromTransaction(txv, i, coin.out, coin.refheight));
            }
        }
        ProduceSignature(DUMMY_SIGNING_PROVIDER, MutableTransactionSignatureCreator(mergedTx, i, coin.out.GetReferenceValue(), 1, SIGHASH_ALL), coin.out.scriptPubKey, sigdata);

        UpdateInput(txin, sigdata);
    }

    return EncodeHexTx(CTransaction(mergedTx));
},
    };
}

static RPCHelpMan signrawtransactionwithkey()
{
    return RPCHelpMan{"signrawtransactionwithkey",
                "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
                "The second argument is an array of base58-encoded private\n"
                "keys that will be the only keys used to sign the transaction.\n"
                "The third optional argument (may be null) is an array of previous transaction outputs that\n"
                "this transaction depends on but may not yet be in the block chain.\n",
                {
                    {"hexstring", RPCArg::Type::STR, RPCArg::Optional::NO, "The transaction hex string"},
                    {"privkeys", RPCArg::Type::ARR, RPCArg::Optional::NO, "The base58-encoded private keys for signing",
                        {
                            {"privatekey", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "private key in base58-encoding"},
                        },
                        },
                    {"prevtxs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "The previous dependent transaction outputs",
                        {
                            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                {
                                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                                    {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                                    {"scriptPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "script key"},
                                    {"redeemScript", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "(required for P2SH) redeem script"},
                                    {"witnessScript", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "(required for P2WSH) witness script"},
                                    {"value", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "(required for Segwit inputs) the amount spent at the reference height of the transaction being spent"},
                                    {"refheight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The lockheight of the transaction output being spent"},
                                },
                                },
                        },
                        },
                    {"sighashtype", RPCArg::Type::STR, RPCArg::Default{"ALL"}, "The signature hash type. Must be one of:\n"
            "       \"DEFAULT\"\n"
            "       \"ALL\"\n"
            "       \"NONE\"\n"
            "       \"SINGLE\"\n"
            "       \"ALL|ANYONECANPAY\"\n"
            "       \"NONE|ANYONECANPAY\"\n"
            "       \"SINGLE|ANYONECANPAY\"\n"
                    },
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "hex", "The hex-encoded raw transaction with signature(s)"},
                        {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                        {RPCResult::Type::ARR, "errors", /*optional=*/true, "Script verification errors (if there are any)",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "txid", "The hash of the referenced, previous transaction"},
                                {RPCResult::Type::NUM, "vout", "The index of the output to spent and used as input"},
                                {RPCResult::Type::ARR, "witness", "",
                                {
                                    {RPCResult::Type::STR_HEX, "witness", ""},
                                }},
                                {RPCResult::Type::STR_HEX, "scriptSig", "The hex-encoded signature script"},
                                {RPCResult::Type::NUM, "sequence", "Script sequence number"},
                                {RPCResult::Type::STR, "error", "Verification or signing error related to the input"},
                            }},
                        }},
                    }
                },
                RPCExamples{
                    HelpExampleCli("signrawtransactionwithkey", "\"myhex\" \"[\\\"key1\\\",\\\"key2\\\"]\"")
            + HelpExampleRpc("signrawtransactionwithkey", "\"myhex\", \"[\\\"key1\\\",\\\"key2\\\"]\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
    }

    FillableSigningProvider keystore;
    const UniValue& keys = request.params[1].get_array();
    for (unsigned int idx = 0; idx < keys.size(); ++idx) {
        UniValue k = keys[idx];
        CKey key = DecodeSecret(k.get_str());
        if (!key.IsValid()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
        }
        keystore.AddKey(key);
    }

    // Fetch previous transactions (inputs):
    std::map<COutPoint, Coin> coins;
    for (const CTxIn& txin : mtx.vin) {
        coins[txin.prevout]; // Create empty map entry keyed by prevout.
    }
    NodeContext& node = EnsureAnyNodeContext(request.context);
    FindCoins(node, coins);

    // Parse the prevtxs array
    ParsePrevouts(request.params[2], &keystore, coins);

    UniValue result(UniValue::VOBJ);
    SignTransaction(mtx, &keystore, coins, request.params[3], result);
    return result;
},
    };
}

const RPCResult decodepst_inputs{
    RPCResult::Type::ARR, "inputs", "",
    {
        {RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::OBJ, "non_witness_utxo", /*optional=*/true, "Decoded network transaction for non-witness UTXOs",
            {
                {RPCResult::Type::ELISION, "",""},
            }},
            {RPCResult::Type::OBJ, "witness_utxo", /*optional=*/true, "Transaction output for witness UTXOs",
            {
                {RPCResult::Type::NUM, "value", "The value in " + CURRENCY_UNIT},
                {RPCResult::Type::NUM, "refheight", "The lockheight of the transaction output being spent"},
                {RPCResult::Type::NUM, "amount", "The value in " + CURRENCY_UNIT + " as input into the PST (at the reference height of the PST)"},
                {RPCResult::Type::OBJ, "scriptPubKey", "",
                {
                    {RPCResult::Type::STR, "asm", "Disassembly of the public key script"},
                    {RPCResult::Type::STR, "desc", "Inferred descriptor for the output"},
                    {RPCResult::Type::STR_HEX, "hex", "The raw public key script bytes, hex-encoded"},
                    {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
                    {RPCResult::Type::STR, "address", /*optional=*/true, "The Freicoin address (only if a well-defined address exists)"},
                }},
            }},
            {RPCResult::Type::OBJ_DYN, "partial_signatures", /*optional=*/true, "",
            {
                {RPCResult::Type::STR, "pubkey", "The public key and signature that corresponds to it."},
            }},
            {RPCResult::Type::STR, "sighash", /*optional=*/true, "The sighash type to be used"},
            {RPCResult::Type::OBJ, "redeem_script", /*optional=*/true, "",
            {
                {RPCResult::Type::STR, "asm", "Disassembly of the redeem script"},
                {RPCResult::Type::STR_HEX, "hex", "The raw redeem script bytes, hex-encoded"},
                {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
            }},
            {RPCResult::Type::NUM, "witscript_version", /*optional=*/true, "The inner script version"},
            {RPCResult::Type::OBJ, "witness_script", /*optional=*/true, "",
            {
                {RPCResult::Type::STR, "asm", /*optional=*/true, "Disassembly of the witness script"},
                {RPCResult::Type::STR_HEX, "hex", "The raw witness script bytes, hex-encoded"},
                {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
            }},
            {RPCResult::Type::ARR, "witness_branch", /*optional=*/true, "The hex-encoded hashes of the Merkle branch proof",
            {
                {RPCResult::Type::STR_HEX, "hash", "The hex-encoded branch skip hash"},
            }},
            {RPCResult::Type::NUM, "witness_path", /*optional=*/true, "The left/right branching information for the Merkle branch proof"},
            {RPCResult::Type::ARR, "bip32_derivs", /*optional=*/true, "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "pubkey", "The public key with the derivation path as the value."},
                    {RPCResult::Type::STR, "master_fingerprint", "The fingerprint of the master key"},
                    {RPCResult::Type::STR, "path", "The path"},
                }},
            }},
            {RPCResult::Type::OBJ, "final_scriptSig", /*optional=*/true, "",
            {
                {RPCResult::Type::STR, "asm", "Disassembly of the final signature script"},
                {RPCResult::Type::STR_HEX, "hex", "The raw final signature script bytes, hex-encoded"},
            }},
            {RPCResult::Type::ARR, "final_scriptwitness", /*optional=*/true, "",
            {
                {RPCResult::Type::STR_HEX, "", "hex-encoded witness data (if any)"},
            }},
            {RPCResult::Type::OBJ_DYN, "ripemd160_preimages", /*optional=*/ true, "",
            {
                {RPCResult::Type::STR_HEX, "hash", "The hash and preimage that corresponds to it."},
            }},
            {RPCResult::Type::OBJ_DYN, "sha256_preimages", /*optional=*/ true, "",
            {
                {RPCResult::Type::STR, "hash", "The hash and preimage that corresponds to it."},
            }},
            {RPCResult::Type::OBJ_DYN, "hash160_preimages", /*optional=*/ true, "",
            {
                {RPCResult::Type::STR, "hash", "The hash and preimage that corresponds to it."},
            }},
            {RPCResult::Type::OBJ_DYN, "hash256_preimages", /*optional=*/ true, "",
            {
                {RPCResult::Type::STR, "hash", "The hash and preimage that corresponds to it."},
            }},
            {RPCResult::Type::OBJ_DYN, "unknown", /*optional=*/ true, "The unknown input fields",
            {
                {RPCResult::Type::STR_HEX, "key", "(key-value pair) An unknown key-value pair"},
            }},
            {RPCResult::Type::ARR, "proprietary", /*optional=*/true, "The input proprietary map",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "identifier", "The hex string for the proprietary identifier"},
                    {RPCResult::Type::NUM, "subtype", "The number for the subtype"},
                    {RPCResult::Type::STR_HEX, "key", "The hex for the key"},
                    {RPCResult::Type::STR_HEX, "value", "The hex for the value"},
                }},
            }},
        }},
    }
};

const RPCResult decodepst_outputs{
    RPCResult::Type::ARR, "outputs", "",
    {
        {RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::OBJ, "redeem_script", /*optional=*/true, "",
            {
                {RPCResult::Type::STR, "asm", "Disassembly of the redeem script"},
                {RPCResult::Type::STR_HEX, "hex", "The raw redeem script bytes, hex-encoded"},
                {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
            }},
            {RPCResult::Type::NUM, "witscript_version", /*optional=*/true, "The inner script version"},
            {RPCResult::Type::OBJ, "witness_script", /*optional=*/true, "",
            {
                {RPCResult::Type::STR, "asm", /*optional=*/true, "Disassembly of the witness script"},
                {RPCResult::Type::STR_HEX, "hex", "The raw witness script bytes, hex-encoded"},
                {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
            }},
            {RPCResult::Type::ARR, "witness_branch", /*optional=*/true, "The hex-encoded hashes of the Merkle branch proof",
            {
                {RPCResult::Type::STR, "hash", "The hex-encoded branch skip hash"},
            }},
            {RPCResult::Type::NUM, "witness_path", /*optional=*/true, "The left/right branching information for the Merkle branch proof"},
            {RPCResult::Type::ARR, "bip32_derivs", /*optional=*/true, "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "pubkey", "The public key this path corresponds to"},
                    {RPCResult::Type::STR, "master_fingerprint", "The fingerprint of the master key"},
                    {RPCResult::Type::STR, "path", "The path"},
                }},
            }},
            {RPCResult::Type::OBJ_DYN, "unknown", /*optional=*/true, "The unknown output fields",
            {
                {RPCResult::Type::STR_HEX, "key", "(key-value pair) An unknown key-value pair"},
            }},
            {RPCResult::Type::ARR, "proprietary", /*optional=*/true, "The output proprietary map",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "identifier", "The hex string for the proprietary identifier"},
                    {RPCResult::Type::NUM, "subtype", "The number for the subtype"},
                    {RPCResult::Type::STR_HEX, "key", "The hex for the key"},
                    {RPCResult::Type::STR_HEX, "value", "The hex for the value"},
                }},
            }},
        }},
    }
};

static RPCHelpMan decodepst()
{
    return RPCHelpMan{
        "decodepst",
        "Return a JSON object representing the serialized, hex-encoded partially signed Freicoin transaction.",
                {
                    {"pst", RPCArg::Type::STR, RPCArg::Optional::NO, "The PST hex string"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::OBJ, "tx", "The decoded network-serialized unsigned transaction.",
                        {
                            {RPCResult::Type::ELISION, "", "The layout is the same as the output of decoderawtransaction."},
                        }},
                        {RPCResult::Type::ARR, "global_xpubs", "",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "xpub", "The extended public key this path corresponds to"},
                                {RPCResult::Type::STR_HEX, "master_fingerprint", "The fingerprint of the master key"},
                                {RPCResult::Type::STR, "path", "The path"},
                            }},
                        }},
                        {RPCResult::Type::NUM, "pst_version", "The PST version number. Not to be confused with the unsigned transaction version"},
                        {RPCResult::Type::ARR, "proprietary", "The global proprietary map",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "identifier", "The hex string for the proprietary identifier"},
                                {RPCResult::Type::NUM, "subtype", "The number for the subtype"},
                                {RPCResult::Type::STR_HEX, "key", "The hex for the key"},
                                {RPCResult::Type::STR_HEX, "value", "The hex for the value"},
                            }},
                        }},
                        {RPCResult::Type::OBJ_DYN, "unknown", "The unknown global fields",
                        {
                             {RPCResult::Type::STR_HEX, "key", "(key-value pair) An unknown key-value pair"},
                        }},
                        decodepst_inputs,
                        decodepst_outputs,
                        {RPCResult::Type::STR_AMOUNT, "demurrage", /*optional=*/true, "The total demurrage of all inputs, if all UTXOs slots in the PST have been filled."},
                        {RPCResult::Type::STR_AMOUNT, "fee", /*optional=*/true, "The transaction fee paid if all UTXOs slots in the PST have been filled."},
                    }
                },
                RPCExamples{
                    HelpExampleCli("decodepst", "\"pst\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Unserialize the transactions
    PartiallySignedTransaction pstx;
    std::string error;
    if (!DecodeHexPST(pstx, request.params[0].get_str(), error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    UniValue result(UniValue::VOBJ);

    // Add the decoded tx
    UniValue tx_univ(UniValue::VOBJ);
    TxToUniv(CTransaction(*pstx.tx), /*block_hash=*/uint256(), /*entry=*/tx_univ, /*include_hex=*/false);
    result.pushKV("tx", tx_univ);

    // Add the global xpubs
    UniValue global_xpubs(UniValue::VARR);
    for (std::pair<KeyOriginInfo, std::set<CExtPubKey>> xpub_pair : pstx.m_xpubs) {
        for (auto& xpub : xpub_pair.second) {
            std::vector<unsigned char> ser_xpub;
            ser_xpub.assign(BIP32_EXTKEY_WITH_VERSION_SIZE, 0);
            xpub.EncodeWithVersion(ser_xpub.data());

            UniValue keypath(UniValue::VOBJ);
            keypath.pushKV("xpub", EncodeBase58Check(ser_xpub));
            keypath.pushKV("master_fingerprint", HexStr(Span<unsigned char>(xpub_pair.first.fingerprint, xpub_pair.first.fingerprint + 4)));
            keypath.pushKV("path", WriteHDKeypath(xpub_pair.first.path));
            global_xpubs.push_back(keypath);
        }
    }
    result.pushKV("global_xpubs", global_xpubs);

    // PST version
    result.pushKV("pst_version", static_cast<uint64_t>(pstx.GetVersion()));

    // Proprietary
    UniValue proprietary(UniValue::VARR);
    for (const auto& entry : pstx.m_proprietary) {
        UniValue this_prop(UniValue::VOBJ);
        this_prop.pushKV("identifier", HexStr(entry.identifier));
        this_prop.pushKV("subtype", entry.subtype);
        this_prop.pushKV("key", HexStr(entry.key));
        this_prop.pushKV("value", HexStr(entry.value));
        proprietary.push_back(this_prop);
    }
    result.pushKV("proprietary", proprietary);

    // Unknown data
    UniValue unknowns(UniValue::VOBJ);
    for (auto entry : pstx.unknown) {
        unknowns.pushKV(HexStr(entry.first), HexStr(entry.second));
    }
    result.pushKV("unknown", unknowns);

    // inputs
    CAmount raw_in = 0;
    CAmount total_in = 0;
    bool have_all_utxos = true;
    UniValue inputs(UniValue::VARR);
    for (unsigned int i = 0; i < pstx.inputs.size(); ++i) {
        const PSTInput& input = pstx.inputs[i];
        UniValue in(UniValue::VOBJ);
        // UTXOs
        bool have_a_utxo = false;
        CTxOut txout;
        CAmount adjusted = 0;
        if (!input.witness_utxo.IsNull()) {
            txout = input.witness_utxo;
            adjusted = txout.GetTimeAdjustedValue(pstx.tx->lock_height - input.witness_refheight);

            UniValue o(UniValue::VOBJ);
            ScriptToUniv(txout.scriptPubKey, /*out=*/o, /*include_hex=*/true, /*include_address=*/true);

            UniValue out(UniValue::VOBJ);
            out.pushKV("value", ValueFromAmount(txout.GetReferenceValue()));
            out.pushKV("refheight", (int64_t)input.witness_refheight);
            out.pushKV("amount", ValueFromAmount(adjusted));
            out.pushKV("scriptPubKey", o);

            in.pushKV("witness_utxo", out);

            have_a_utxo = true;
        }
        if (input.non_witness_utxo) {
            txout = input.non_witness_utxo->vout[pstx.tx->vin[i].prevout.n];
            adjusted = txout.GetTimeAdjustedValue(pstx.tx->lock_height - input.non_witness_utxo->lock_height);

            UniValue non_wit(UniValue::VOBJ);
            TxToUniv(*input.non_witness_utxo, /*block_hash=*/uint256(), /*entry=*/non_wit, /*include_hex=*/false);
            in.pushKV("non_witness_utxo", non_wit);

            have_a_utxo = true;
        }
        if (have_a_utxo) {
            if (MoneyRange(txout.GetReferenceValue()) && MoneyRange(adjusted) && MoneyRange(total_in + adjusted)) {
                raw_in += txout.GetReferenceValue();
                total_in += adjusted;
            } else {
                // Hack to just not show fee later
                have_all_utxos = false;
            }
        } else {
            have_all_utxos = false;
        }

        // Partial sigs
        if (!input.partial_sigs.empty()) {
            UniValue partial_sigs(UniValue::VOBJ);
            for (const auto& sig : input.partial_sigs) {
                partial_sigs.pushKV(HexStr(sig.second.first), HexStr(sig.second.second));
            }
            in.pushKV("partial_signatures", partial_sigs);
        }

        // Sighash
        if (input.sighash_type != std::nullopt) {
            in.pushKV("sighash", SighashToStr((unsigned char)*input.sighash_type));
        }

        // Redeem script and witness script
        if (!input.redeem_script.empty()) {
            UniValue r(UniValue::VOBJ);
            ScriptToUniv(input.redeem_script, /*out=*/r);
            in.pushKV("redeem_script", r);
        }
        if (!input.witness_entry.m_script.empty()) {
            in.pushKV("witscript_version", (int64_t)input.witness_entry.m_script[0]);
            UniValue r(UniValue::VOBJ);
            if (input.witness_entry.m_script[0] == 0x00) {
                ScriptToUniv(CScript(input.witness_entry.m_script.begin() + 1, input.witness_entry.m_script.end()), /*out=*/r);
            } else {
                r.pushKV("hex", HexStr({&input.witness_entry.m_script[1], input.witness_entry.m_script.size() - 1}));
                r.pushKV("type", "unknown");
            }
            in.pushKV("witness_script", r);
            UniValue branch(UniValue::VARR);
            for (const auto& hash : input.witness_entry.m_branch) {
                branch.push_back(HexStr(hash));
            }
            in.pushKV("witness_branch", branch);
            in.pushKV("witness_path", (int64_t)input.witness_entry.m_path);
        }

        // keypaths
        if (!input.hd_keypaths.empty()) {
            UniValue keypaths(UniValue::VARR);
            for (auto entry : input.hd_keypaths) {
                UniValue keypath(UniValue::VOBJ);
                keypath.pushKV("pubkey", HexStr(entry.first));

                keypath.pushKV("master_fingerprint", strprintf("%08x", ReadBE32(entry.second.fingerprint)));
                keypath.pushKV("path", WriteHDKeypath(entry.second.path));
                keypaths.push_back(keypath);
            }
            in.pushKV("bip32_derivs", keypaths);
        }

        // Final scriptSig and scriptwitness
        if (!input.final_script_sig.empty()) {
            UniValue scriptsig(UniValue::VOBJ);
            scriptsig.pushKV("asm", ScriptToAsmStr(input.final_script_sig, true));
            scriptsig.pushKV("hex", HexStr(input.final_script_sig));
            in.pushKV("final_scriptSig", scriptsig);
        }
        if (!input.final_script_witness.IsNull()) {
            UniValue txinwitness(UniValue::VARR);
            for (const auto& item : input.final_script_witness.stack) {
                txinwitness.push_back(HexStr(item));
            }
            in.pushKV("final_scriptwitness", txinwitness);
        }

        // Ripemd160 hash preimages
        if (!input.ripemd160_preimages.empty()) {
            UniValue ripemd160_preimages(UniValue::VOBJ);
            for (const auto& [hash, preimage] : input.ripemd160_preimages) {
                ripemd160_preimages.pushKV(HexStr(hash), HexStr(preimage));
            }
            in.pushKV("ripemd160_preimages", ripemd160_preimages);
        }

        // Sha256 hash preimages
        if (!input.sha256_preimages.empty()) {
            UniValue sha256_preimages(UniValue::VOBJ);
            for (const auto& [hash, preimage] : input.sha256_preimages) {
                sha256_preimages.pushKV(HexStr(hash), HexStr(preimage));
            }
            in.pushKV("sha256_preimages", sha256_preimages);
        }

        // Hash160 hash preimages
        if (!input.hash160_preimages.empty()) {
            UniValue hash160_preimages(UniValue::VOBJ);
            for (const auto& [hash, preimage] : input.hash160_preimages) {
                hash160_preimages.pushKV(HexStr(hash), HexStr(preimage));
            }
            in.pushKV("hash160_preimages", hash160_preimages);
        }

        // Hash256 hash preimages
        if (!input.hash256_preimages.empty()) {
            UniValue hash256_preimages(UniValue::VOBJ);
            for (const auto& [hash, preimage] : input.hash256_preimages) {
                hash256_preimages.pushKV(HexStr(hash), HexStr(preimage));
            }
            in.pushKV("hash256_preimages", hash256_preimages);
        }

        // Proprietary
        if (!input.m_proprietary.empty()) {
            UniValue proprietary(UniValue::VARR);
            for (const auto& entry : input.m_proprietary) {
                UniValue this_prop(UniValue::VOBJ);
                this_prop.pushKV("identifier", HexStr(entry.identifier));
                this_prop.pushKV("subtype", entry.subtype);
                this_prop.pushKV("key", HexStr(entry.key));
                this_prop.pushKV("value", HexStr(entry.value));
                proprietary.push_back(this_prop);
            }
            in.pushKV("proprietary", proprietary);
        }

        // Unknown data
        if (input.unknown.size() > 0) {
            UniValue unknowns(UniValue::VOBJ);
            for (auto entry : input.unknown) {
                unknowns.pushKV(HexStr(entry.first), HexStr(entry.second));
            }
            in.pushKV("unknown", unknowns);
        }

        inputs.push_back(in);
    }
    result.pushKV("inputs", inputs);

    // outputs
    CAmount output_value = 0;
    UniValue outputs(UniValue::VARR);
    for (unsigned int i = 0; i < pstx.outputs.size(); ++i) {
        const PSTOutput& output = pstx.outputs[i];
        UniValue out(UniValue::VOBJ);
        // Redeem script and witness script
        if (!output.redeem_script.empty()) {
            UniValue r(UniValue::VOBJ);
            ScriptToUniv(output.redeem_script, /*out=*/r);
            out.pushKV("redeem_script", r);
        }
        if (!output.witness_entry.m_script.empty()) {
            out.pushKV("witscript_version", (int64_t)output.witness_entry.m_script[0]);
            UniValue r(UniValue::VOBJ);
            if (output.witness_entry.m_script[0] == 0x00) {
                ScriptToUniv(CScript(output.witness_entry.m_script.begin() + 1, output.witness_entry.m_script.end()), /*out=*/r);
            } else {
                r.pushKV("hex", HexStr({&output.witness_entry.m_script[1], output.witness_entry.m_script.size() - 1}));
                r.pushKV("type", "unknown");
            }
            out.pushKV("witness_script", r);
            UniValue branch(UniValue::VARR);
            for (const auto& hash : output.witness_entry.m_branch) {
                branch.push_back(HexStr(hash));
            }
            out.pushKV("witness_branch", branch);
            out.pushKV("witness_path", (int64_t)output.witness_entry.m_path);
        }

        // keypaths
        if (!output.hd_keypaths.empty()) {
            UniValue keypaths(UniValue::VARR);
            for (auto entry : output.hd_keypaths) {
                UniValue keypath(UniValue::VOBJ);
                keypath.pushKV("pubkey", HexStr(entry.first));
                keypath.pushKV("master_fingerprint", strprintf("%08x", ReadBE32(entry.second.fingerprint)));
                keypath.pushKV("path", WriteHDKeypath(entry.second.path));
                keypaths.push_back(keypath);
            }
            out.pushKV("bip32_derivs", keypaths);
        }

        // Proprietary
        if (!output.m_proprietary.empty()) {
            UniValue proprietary(UniValue::VARR);
            for (const auto& entry : output.m_proprietary) {
                UniValue this_prop(UniValue::VOBJ);
                this_prop.pushKV("identifier", HexStr(entry.identifier));
                this_prop.pushKV("subtype", entry.subtype);
                this_prop.pushKV("key", HexStr(entry.key));
                this_prop.pushKV("value", HexStr(entry.value));
                proprietary.push_back(this_prop);
            }
            out.pushKV("proprietary", proprietary);
        }

        // Unknown data
        if (output.unknown.size() > 0) {
            UniValue unknowns(UniValue::VOBJ);
            for (auto entry : output.unknown) {
                unknowns.pushKV(HexStr(entry.first), HexStr(entry.second));
            }
            out.pushKV("unknown", unknowns);
        }

        outputs.push_back(out);

        // Fee calculation
        CAmount txout_value = pstx.tx->vout[i].GetReferenceValue();
        if (MoneyRange(txout_value) && MoneyRange(output_value + txout_value)) {
            output_value += txout_value;
        } else {
            // Hack to just not show fee later
            have_all_utxos = false;
        }
    }
    result.pushKV("outputs", outputs);
    if (have_all_utxos) {
        result.pushKV("demurrage", ValueFromAmount(raw_in - total_in));
        result.pushKV("fee", ValueFromAmount(total_in - output_value));
    }

    return result;
},
    };
}

static RPCHelpMan combinepst()
{
    return RPCHelpMan{"combinepst",
                "\nCombine multiple partially signed Freicoin transactions into one transaction.\n"
                "Implements the Combiner role.\n",
                {
                    {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The hex strings of partially signed transactions",
                        {
                            {"pst", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A hex string of a PST"},
                        },
                        },
                },
                RPCResult{
                    RPCResult::Type::STR, "", "The hex-encoded partially signed transaction"
                },
                RPCExamples{
                    HelpExampleCli("combinepst", R"('["myhex_1", "myhex_2", "myhex_3"]')")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Unserialize the transactions
    std::vector<PartiallySignedTransaction> pstxs;
    UniValue txs = request.params[0].get_array();
    if (txs.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter 'txs' cannot be empty");
    }
    for (unsigned int i = 0; i < txs.size(); ++i) {
        PartiallySignedTransaction pstx;
        std::string error;
        if (!DecodeHexPST(pstx, txs[i].get_str(), error)) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
        }
        pstxs.push_back(pstx);
    }

    PartiallySignedTransaction merged_pst;
    const TransactionError error = CombinePSTs(merged_pst, pstxs);
    if (error != TransactionError::OK) {
        throw JSONRPCTransactionError(error);
    }

    DataStream ssTx{};
    ssTx << merged_pst;
    return HexStr(ssTx);
},
    };
}

static RPCHelpMan finalizepst()
{
    return RPCHelpMan{"finalizepst",
                "Finalize the inputs of a PST. If the transaction is fully signed, it will produce a\n"
                "network serialized transaction which can be broadcast with sendrawtransaction. Otherwise a PST will be\n"
                "created which has the final_scriptSig and final_scriptWitness fields filled for inputs that are complete.\n"
                "Implements the Finalizer and Extractor roles.\n",
                {
                    {"pst", RPCArg::Type::STR, RPCArg::Optional::NO, "A hex string of a PST"},
                    {"extract", RPCArg::Type::BOOL, RPCArg::Default{true}, "If true and the transaction is complete,\n"
            "                             extract and return the complete transaction in normal network serialization instead of the PST."},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "pst", /*optional=*/true, "The hex-encoded partially signed transaction if not extracted"},
                        {RPCResult::Type::STR_HEX, "hex", /*optional=*/true, "The hex-encoded network transaction if extracted"},
                        {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("finalizepst", "\"pst\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Unserialize the transactions
    PartiallySignedTransaction pstx;
    std::string error;
    if (!DecodeHexPST(pstx, request.params[0].get_str(), error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    bool extract = request.params[1].isNull() || (!request.params[1].isNull() && request.params[1].get_bool());

    CMutableTransaction mtx;
    bool complete = FinalizeAndExtractPST(pstx, mtx);

    UniValue result(UniValue::VOBJ);
    DataStream ssTx{};
    std::string result_str;

    if (complete && extract) {
        ssTx << TX_WITH_WITNESS(mtx);
        result_str = HexStr(ssTx);
        result.pushKV("hex", result_str);
    } else {
        ssTx << pstx;
        result.pushKV("pst", HexStr(ssTx.str()));
    }
    result.pushKV("complete", complete);

    return result;
},
    };
}

static RPCHelpMan createpst()
{
    return RPCHelpMan{"createpst",
                "\nCreates a transaction in the Partially Signed Transaction format.\n"
                "Implements the Creator role.\n",
                CreateTxDoc(),
                RPCResult{
                    RPCResult::Type::STR, "", "The resulting raw transaction (hex-encoded string)"
                },
                RPCExamples{
                    HelpExampleCli("createpst", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    int height = -1;
    {
        LOCK(cs_main);
        height = chainman.ActiveChain().Height();
    }

    std::optional<bool> rbf;
    if (!request.params[3].isNull()) {
        rbf = request.params[3].get_bool();
    }
    CMutableTransaction rawTx = ConstructTransaction(request.params[0], request.params[1], request.params[2], request.params[3], height, rbf);

    // Make a blank pst
    PartiallySignedTransaction pstx;
    pstx.tx = rawTx;
    for (unsigned int i = 0; i < rawTx.vin.size(); ++i) {
        pstx.inputs.emplace_back();
    }
    for (unsigned int i = 0; i < rawTx.vout.size(); ++i) {
        pstx.outputs.emplace_back();
    }

    // Serialize the PST
    DataStream ssTx{};
    ssTx << pstx;

    return HexStr(ssTx);
},
    };
}

static RPCHelpMan converttopst()
{
    return RPCHelpMan{"converttopst",
                "\nConverts a network serialized transaction to a PST. This should be used only with createrawtransaction and fundrawtransaction\n"
                "createpst and walletcreatefundedpst should be used for new applications.\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of a raw transaction"},
                    {"permitsigdata", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, any signatures in the input will be discarded and conversion\n"
                            "                              will continue. If false, RPC will fail if any signatures are present."},
                    {"iswitness", RPCArg::Type::BOOL, RPCArg::DefaultHint{"depends on heuristic tests"}, "Whether the transaction hex is a serialized witness transaction.\n"
                        "If iswitness is not present, heuristic tests will be used in decoding.\n"
                        "If true, only witness deserialization will be tried.\n"
                        "If false, only non-witness deserialization will be tried.\n"
                        "This boolean should reflect whether the transaction has inputs\n"
                        "(e.g. fully valid, or on-chain transactions), if known by the caller."
                    },
                },
                RPCResult{
                    RPCResult::Type::STR, "", "The resulting raw transaction (hex-encoded string)"
                },
                RPCExamples{
                            "\nCreate a transaction\n"
                            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"") +
                            "\nConvert the transaction to a PST\n"
                            + HelpExampleCli("converttopst", "\"rawtransaction\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // parse hex string from parameter
    CMutableTransaction tx;
    bool permitsigdata = request.params[1].isNull() ? false : request.params[1].get_bool();
    bool witness_specified = !request.params[2].isNull();
    bool iswitness = witness_specified ? request.params[2].get_bool() : false;
    const bool try_witness = witness_specified ? iswitness : true;
    const bool try_no_witness = witness_specified ? !iswitness : true;
    if (!DecodeHexTx(tx, request.params[0].get_str(), try_no_witness, try_witness)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    // Remove all scriptSigs and scriptWitnesses from inputs
    for (CTxIn& input : tx.vin) {
        if ((!input.scriptSig.empty() || !input.scriptWitness.IsNull()) && !permitsigdata) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Inputs must not have scriptSigs and scriptWitnesses");
        }
        input.scriptSig.clear();
        input.scriptWitness.SetNull();
    }

    // Make a blank pst
    PartiallySignedTransaction pstx;
    pstx.tx = tx;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        pstx.inputs.emplace_back();
    }
    for (unsigned int i = 0; i < tx.vout.size(); ++i) {
        pstx.outputs.emplace_back();
    }

    // Serialize the PST
    DataStream ssTx{};
    ssTx << pstx;

    return HexStr(ssTx);
},
    };
}

static RPCHelpMan utxoupdatepst()
{
    return RPCHelpMan{"utxoupdatepst",
            "\nUpdates all segwit inputs and outputs in a PST with data from output descriptors, the UTXO set, txindex, or the mempool.\n",
            {
                {"pst", RPCArg::Type::STR, RPCArg::Optional::NO, "A hex string of a PST"},
                {"descriptors", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "An array of either strings or objects", {
                    {"", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "An output descriptor"},
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "An object with an output descriptor and extra information", {
                         {"desc", RPCArg::Type::STR, RPCArg::Optional::NO, "An output descriptor"},
                         {"range", RPCArg::Type::RANGE, RPCArg::Default{1000}, "Up to what index HD chains should be explored (either end or [begin,end])"},
                    }},
                }},
            },
            RPCResult {
                    RPCResult::Type::STR, "", "The hex-encoded partially signed transaction with inputs updated"
            },
            RPCExamples {
                HelpExampleCli("utxoupdatepst", "\"pst\"")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Parse descriptors, if any.
    FlatSigningProvider provider;
    if (!request.params[1].isNull()) {
        auto descs = request.params[1].get_array();
        for (size_t i = 0; i < descs.size(); ++i) {
            EvalDescriptorStringOrObject(descs[i], provider);
        }
    }

    // We don't actually need private keys further on; hide them as a precaution.
    const PartiallySignedTransaction& pstx = ProcessPST(
        request.params[0].get_str(),
        request.context,
        HidingSigningProvider(&provider, /*hide_secret=*/true, /*hide_origin=*/false),
        /*sighash_type=*/SIGHASH_ALL,
        /*finalize=*/false);

    DataStream ssTx{};
    ssTx << pstx;
    return HexStr(ssTx);
},
    };
}

static RPCHelpMan joinpsts()
{
    return RPCHelpMan{"joinpsts",
            "\nJoins multiple distinct PSTs with different inputs and outputs into one PST with inputs and outputs from all of the PSTs\n"
            "No input in any of the PSTs can be in more than one of the PSTs.\n",
            {
                {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The hex strings of partially signed transactions",
                    {
                        {"pst", RPCArg::Type::STR, RPCArg::Optional::NO, "A hex string of a PST"}
                    }}
            },
            RPCResult {
                    RPCResult::Type::STR, "", "The hex-encoded partially signed transaction"
            },
            RPCExamples {
                HelpExampleCli("joinpsts", "\"pst\"")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Unserialize the transactions
    std::vector<PartiallySignedTransaction> pstxs;
    UniValue txs = request.params[0].get_array();

    if (txs.size() <= 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "At least two PSTs are required to join PSTs.");
    }

    uint32_t best_version = 1;
    uint32_t best_locktime = 0xffffffff;
    for (unsigned int i = 0; i < txs.size(); ++i) {
        PartiallySignedTransaction pstx;
        std::string error;
        if (!DecodeHexPST(pstx, txs[i].get_str(), error)) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
        }
        pstxs.push_back(pstx);
        // Choose the highest version number
        if (static_cast<uint32_t>(pstx.tx->nVersion) > best_version) {
            best_version = static_cast<uint32_t>(pstx.tx->nVersion);
        }
        // Choose the lowest lock time
        if (pstx.tx->nLockTime < best_locktime) {
            best_locktime = pstx.tx->nLockTime;
        }
    }

    // Create a blank pst where everything will be added
    PartiallySignedTransaction merged_pst;
    merged_pst.tx = CMutableTransaction();
    merged_pst.tx->nVersion = static_cast<int32_t>(best_version);
    merged_pst.tx->nLockTime = best_locktime;

    // Merge
    for (auto& pst : pstxs) {
        for (unsigned int i = 0; i < pst.tx->vin.size(); ++i) {
            if (!merged_pst.AddInput(pst.tx->vin[i], pst.inputs[i])) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Input %s:%d exists in multiple PSTs", pst.tx->vin[i].prevout.hash.ToString(), pst.tx->vin[i].prevout.n));
            }
        }
        for (unsigned int i = 0; i < pst.tx->vout.size(); ++i) {
            merged_pst.AddOutput(pst.tx->vout[i], pst.outputs[i]);
        }
        for (auto& xpub_pair : pst.m_xpubs) {
            if (merged_pst.m_xpubs.count(xpub_pair.first) == 0) {
                merged_pst.m_xpubs[xpub_pair.first] = xpub_pair.second;
            } else {
                merged_pst.m_xpubs[xpub_pair.first].insert(xpub_pair.second.begin(), xpub_pair.second.end());
            }
        }
        merged_pst.unknown.insert(pst.unknown.begin(), pst.unknown.end());
    }

    // Generate list of shuffled indices for shuffling inputs and outputs of the merged PST
    std::vector<int> input_indices(merged_pst.inputs.size());
    std::iota(input_indices.begin(), input_indices.end(), 0);
    std::vector<int> output_indices(merged_pst.outputs.size());
    std::iota(output_indices.begin(), output_indices.end(), 0);

    // Shuffle input and output indices lists
    Shuffle(input_indices.begin(), input_indices.end(), FastRandomContext());
    Shuffle(output_indices.begin(), output_indices.end(), FastRandomContext());

    PartiallySignedTransaction shuffled_pst;
    shuffled_pst.tx = CMutableTransaction();
    shuffled_pst.tx->nVersion = merged_pst.tx->nVersion;
    shuffled_pst.tx->nLockTime = merged_pst.tx->nLockTime;
    for (int i : input_indices) {
        shuffled_pst.AddInput(merged_pst.tx->vin[i], merged_pst.inputs[i]);
    }
    for (int i : output_indices) {
        shuffled_pst.AddOutput(merged_pst.tx->vout[i], merged_pst.outputs[i]);
    }
    shuffled_pst.unknown.insert(merged_pst.unknown.begin(), merged_pst.unknown.end());

    DataStream ssTx{};
    ssTx << shuffled_pst;
    return HexStr(ssTx);
},
    };
}

static RPCHelpMan analyzepst()
{
    return RPCHelpMan{"analyzepst",
            "\nAnalyzes and provides information about the current status of a PST and its inputs\n",
            {
                {"pst", RPCArg::Type::STR, RPCArg::Optional::NO, "A hex string of a PST"}
            },
            RPCResult {
                RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::ARR, "inputs", /*optional=*/true, "",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::BOOL, "has_utxo", "Whether a UTXO is provided"},
                            {RPCResult::Type::BOOL, "is_final", "Whether the input is finalized"},
                            {RPCResult::Type::OBJ, "missing", /*optional=*/true, "Things that are missing that are required to complete this input",
                            {
                                {RPCResult::Type::ARR, "pubkeys", /*optional=*/true, "",
                                {
                                    {RPCResult::Type::STR_HEX, "keyid", "Public key ID, hash160 of the public key, of a public key whose BIP 32 derivation path is missing"},
                                }},
                                {RPCResult::Type::ARR, "signatures", /*optional=*/true, "",
                                {
                                    {RPCResult::Type::STR_HEX, "keyid", "Public key ID, hash160 of the public key, of a public key whose signature is missing"},
                                }},
                                {RPCResult::Type::STR_HEX, "redeemscript", /*optional=*/true, "Hash160 of the redeemScript that is missing"},
                                {RPCResult::Type::STR_HEX, "witnessscript", /*optional=*/true, "SHA256 of the witnessScript that is missing"},
                            }},
                            {RPCResult::Type::STR, "next", /*optional=*/true, "Role of the next person that this input needs to go to"},
                        }},
                    }},
                    {RPCResult::Type::NUM, "estimated_vsize", /*optional=*/true, "Estimated vsize of the final signed transaction"},
                    {RPCResult::Type::STR_AMOUNT, "estimated_feerate", /*optional=*/true, "Estimated feerate of the final signed transaction in " + CURRENCY_UNIT + "/kvB. Shown only if all UTXO slots in the PST have been filled"},
                    {RPCResult::Type::STR_AMOUNT, "demurrage", /*optional=*/true, "The total input lost to demurrage. Shown only if all UTXO slots in the PST that have been filled"},
                    {RPCResult::Type::STR_AMOUNT, "fee", /*optional=*/true, "The transaction fee paid. Shown only if all UTXO slots in the PST have been filled"},
                    {RPCResult::Type::STR, "next", "Role of the next person that this pst needs to go to"},
                    {RPCResult::Type::STR, "error", /*optional=*/true, "Error message (if there is one)"},
                }
            },
            RPCExamples {
                HelpExampleCli("analyzepst", "\"pst\"")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Unserialize the transaction
    PartiallySignedTransaction pstx;
    std::string error;
    if (!DecodeHexPST(pstx, request.params[0].get_str(), error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    PSTAnalysis psta = AnalyzePST(pstx);

    UniValue result(UniValue::VOBJ);
    UniValue inputs_result(UniValue::VARR);
    for (const auto& input : psta.inputs) {
        UniValue input_univ(UniValue::VOBJ);
        UniValue missing(UniValue::VOBJ);

        input_univ.pushKV("has_utxo", input.has_utxo);
        input_univ.pushKV("is_final", input.is_final);
        input_univ.pushKV("next", PSTRoleName(input.next));

        if (!input.missing_pubkeys.empty()) {
            UniValue missing_pubkeys_univ(UniValue::VARR);
            for (const CKeyID& pubkey : input.missing_pubkeys) {
                missing_pubkeys_univ.push_back(HexStr(pubkey));
            }
            missing.pushKV("pubkeys", missing_pubkeys_univ);
        }
        if (!input.missing_redeem_script.IsNull()) {
            missing.pushKV("redeemscript", HexStr(input.missing_redeem_script));
        }
        if (!input.missing_witness_script.IsNull()) {
            missing.pushKV("witnessscript", HexStr(input.missing_witness_script));
        }
        if (!input.missing_sigs.empty()) {
            UniValue missing_sigs_univ(UniValue::VARR);
            for (const CKeyID& pubkey : input.missing_sigs) {
                missing_sigs_univ.push_back(HexStr(pubkey));
            }
            missing.pushKV("signatures", missing_sigs_univ);
        }
        if (!missing.getKeys().empty()) {
            input_univ.pushKV("missing", missing);
        }
        inputs_result.push_back(input_univ);
    }
    if (!inputs_result.empty()) result.pushKV("inputs", inputs_result);

    if (psta.estimated_vsize != std::nullopt) {
        result.pushKV("estimated_vsize", (int)*psta.estimated_vsize);
    }
    if (psta.estimated_feerate != std::nullopt) {
        result.pushKV("estimated_feerate", ValueFromAmount(psta.estimated_feerate->GetFeePerK()));
    }
    if (psta.demurrage != std::nullopt) {
        result.pushKV("demurrage", ValueFromAmount(*psta.demurrage));
    }
    if (psta.fee != std::nullopt) {
        result.pushKV("fee", ValueFromAmount(*psta.fee));
    }
    result.pushKV("next", PSTRoleName(psta.next));
    if (!psta.error.empty()) {
        result.pushKV("error", psta.error);
    }

    return result;
},
    };
}

RPCHelpMan descriptorprocesspst()
{
    return RPCHelpMan{"descriptorprocesspst",
                "\nUpdate all segwit inputs in a PST with information from output descriptors, the UTXO set or the mempool. \n"
                "Then, sign the inputs we are able to with information from the output descriptors. ",
                {
                    {"pst", RPCArg::Type::STR, RPCArg::Optional::NO, "The transaction, hex-encoded"},
                    {"descriptors", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of either strings or objects", {
                        {"", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "An output descriptor"},
                        {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "An object with an output descriptor and extra information", {
                             {"desc", RPCArg::Type::STR, RPCArg::Optional::NO, "An output descriptor"},
                             {"range", RPCArg::Type::RANGE, RPCArg::Default{1000}, "Up to what index HD chains should be explored (either end or [begin,end])"},
                        }},
                    }},
                    {"sighashtype", RPCArg::Type::STR, RPCArg::Default{"DEFAULT for Taproot, ALL otherwise"}, "The signature hash type to sign with if not specified by the PST. Must be one of\n"
            "       \"DEFAULT\"\n"
            "       \"ALL\"\n"
            "       \"NONE\"\n"
            "       \"SINGLE\"\n"
            "       \"ALL|ANYONECANPAY\"\n"
            "       \"NONE|ANYONECANPAY\"\n"
            "       \"SINGLE|ANYONECANPAY\""},
                    {"bip32derivs", RPCArg::Type::BOOL, RPCArg::Default{true}, "Include BIP 32 derivation paths for public keys if we know them"},
                    {"finalize", RPCArg::Type::BOOL, RPCArg::Default{true}, "Also finalize inputs if possible"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "pst", "The hex-encoded partially signed transaction"},
                        {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                        {RPCResult::Type::STR_HEX, "hex", /*optional=*/true, "The hex-encoded network transaction if complete"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("descriptorprocesspst", "\"pst\" \"[\\\"descriptor1\\\", \\\"descriptor2\\\"]\"") +
                    HelpExampleCli("descriptorprocesspst", "\"pst\" \"[{\\\"desc\\\":\\\"mydescriptor\\\", \\\"range\\\":21}]\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Add descriptor information to a signing provider
    FlatSigningProvider provider;

    auto descs = request.params[1].get_array();
    for (size_t i = 0; i < descs.size(); ++i) {
        EvalDescriptorStringOrObject(descs[i], provider, /*expand_priv=*/true);
    }

    int sighash_type = ParseSighashString(request.params[2]);
    bool bip32derivs = request.params[3].isNull() ? true : request.params[3].get_bool();
    bool finalize = request.params[4].isNull() ? true : request.params[4].get_bool();

    const PartiallySignedTransaction& pstx = ProcessPST(
        request.params[0].get_str(),
        request.context,
        HidingSigningProvider(&provider, /*hide_secret=*/false, !bip32derivs),
        sighash_type,
        finalize);

    // Check whether or not all of the inputs are now signed
    bool complete = true;
    for (const auto& input : pstx.inputs) {
        complete &= PSTInputSigned(input);
    }

    DataStream ssTx{};
    ssTx << pstx;

    UniValue result(UniValue::VOBJ);

    result.pushKV("pst", HexStr(ssTx));
    result.pushKV("complete", complete);
    if (complete) {
        CMutableTransaction mtx;
        PartiallySignedTransaction pstx_copy = pstx;
        CHECK_NONFATAL(FinalizeAndExtractPST(pstx_copy, mtx));
        DataStream ssTx_final;
        ssTx_final << TX_WITH_WITNESS(mtx);
        result.pushKV("hex", HexStr(ssTx_final));
    }
    return result;
},
    };
}

void RegisterRawTransactionRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"rawtransactions", &getrawtransaction},
        {"rawtransactions", &createrawtransaction},
        {"rawtransactions", &decoderawtransaction},
        {"rawtransactions", &decodescript},
        {"rawtransactions", &combinerawtransaction},
        {"rawtransactions", &signrawtransactionwithkey},
        {"rawtransactions", &decodepst},
        {"rawtransactions", &combinepst},
        {"rawtransactions", &finalizepst},
        {"rawtransactions", &createpst},
        {"rawtransactions", &converttopst},
        {"rawtransactions", &utxoupdatepst},
        {"rawtransactions", &descriptorprocesspst},
        {"rawtransactions", &joinpsts},
        {"rawtransactions", &analyzepst},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
