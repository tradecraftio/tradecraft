// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
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

#include <rpc/rawtransaction_util.h>

#include <coins.h>
#include <core_io.h>
#include <key_io.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <rpc/request.h>
#include <rpc/util.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/strencodings.h>

CMutableTransaction ConstructTransaction(const UniValue& inputs_in, const UniValue& outputs_in, const UniValue& locktime, const UniValue& lockheight, int current_height)
{
    if (inputs_in.isNull() || outputs_in.isNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

    UniValue inputs = inputs_in.get_array();
    const bool outputs_is_obj = outputs_in.isObject();
    UniValue outputs = outputs_is_obj ? outputs_in.get_obj() : outputs_in.get_array();

    CMutableTransaction rawTx;

    if (!locktime.isNull()) {
        int64_t nLockTime = locktime.get_int64();
        if (nLockTime < 0 || nLockTime > LOCKTIME_MAX)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, locktime out of range");
        rawTx.nLockTime = nLockTime;
    }

    rawTx.lock_height = 0;
    if (!lockheight.isNull()) {
        rawTx.lock_height = lockheight.get_int();
    }
    if (rawTx.lock_height <= 0) {
        rawTx.lock_height = current_height + 1;
    }

    for (unsigned int idx = 0; idx < inputs.size(); idx++) {
        const UniValue& input = inputs[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        uint32_t nSequence;
        if (rawTx.nLockTime) {
            nSequence = CTxIn::SEQUENCE_FINAL - 1;
        } else {
            nSequence = CTxIn::SEQUENCE_FINAL;
        }

        // set the sequence number if passed in the parameters object
        const UniValue& sequenceObj = find_value(o, "sequence");
        if (sequenceObj.isNum()) {
            int64_t seqNr64 = sequenceObj.get_int64();
            if (seqNr64 < 0 || seqNr64 > CTxIn::SEQUENCE_FINAL) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
            } else {
                nSequence = (uint32_t)seqNr64;
            }
        }

        CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);

        rawTx.vin.push_back(in);
    }

    if (!outputs_is_obj) {
        // Translate array of key-value pairs into dict
        UniValue outputs_dict = UniValue(UniValue::VOBJ);
        for (size_t i = 0; i < outputs.size(); ++i) {
            const UniValue& output = outputs[i];
            if (!output.isObject()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, key-value pair not an object as expected");
            }
            if (output.size() != 1) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, key-value pair must contain exactly one key");
            }
            outputs_dict.pushKVs(output);
        }
        outputs = std::move(outputs_dict);
    }

    // Duplicate checking
    std::set<CTxDestination> destinations;
    bool has_data{false};

    for (const std::string& name_ : outputs.getKeys()) {
        if (name_ == "data") {
            if (has_data) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, duplicate key: data");
            }
            has_data = true;
            std::vector<unsigned char> data = ParseHexV(outputs[name_].getValStr(), "Data");

            CTxOut out(0, CScript() << OP_RETURN << data);
            rawTx.vout.push_back(out);
        } else {
            CTxDestination destination = DecodeDestination(name_);
            if (!IsValidDestination(destination)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Freicoin address: ") + name_);
            }

            if (!destinations.insert(destination).second) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ") + name_);
            }

            CScript scriptPubKey = GetScriptForDestination(destination);
            CAmount nAmount = AmountFromValue(outputs[name_]);

            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        }
    }

    return rawTx;
}

/** Pushes a JSON object for script verification or signing errors to vErrorsRet. */
static void TxInErrorToJSON(const CTxIn& txin, UniValue& vErrorsRet, const std::string& strMessage)
{
    UniValue entry(UniValue::VOBJ);
    entry.pushKV("txid", txin.prevout.hash.ToString());
    entry.pushKV("vout", (uint64_t)txin.prevout.n);
    UniValue witness(UniValue::VARR);
    for (unsigned int i = 0; i < txin.scriptWitness.stack.size(); i++) {
        witness.push_back(HexStr(txin.scriptWitness.stack[i].begin(), txin.scriptWitness.stack[i].end()));
    }
    entry.pushKV("witness", witness);
    entry.pushKV("scriptSig", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
    entry.pushKV("sequence", (uint64_t)txin.nSequence);
    entry.pushKV("error", strMessage);
    vErrorsRet.push_back(entry);
}

void ParsePrevouts(const UniValue& prevTxsUnival, FillableSigningProvider* keystore, std::map<COutPoint, Coin>& coins)
{
    if (!prevTxsUnival.isNull()) {
        UniValue prevTxs = prevTxsUnival.get_array();
        for (unsigned int idx = 0; idx < prevTxs.size(); ++idx) {
            const UniValue& p = prevTxs[idx];
            if (!p.isObject()) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");
            }

            UniValue prevOut = p.get_obj();

            RPCTypeCheckObj(prevOut,
                {
                    {"txid", UniValueType(UniValue::VSTR)},
                    {"vout", UniValueType(UniValue::VNUM)},
                    {"scriptPubKey", UniValueType(UniValue::VSTR)},
                });

            uint256 txid = ParseHashO(prevOut, "txid");

            int nOut = find_value(prevOut, "vout").get_int();
            if (nOut < 0) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");
            }

            COutPoint out(txid, nOut);
            std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            {
                auto coin = coins.find(out);
                if (coin != coins.end() && !coin->second.IsSpent() && coin->second.out.scriptPubKey != scriptPubKey) {
                    std::string err("Previous output scriptPubKey mismatch:\n");
                    err = err + ScriptToAsmStr(coin->second.out.scriptPubKey) + "\nvs:\n"+
                        ScriptToAsmStr(scriptPubKey);
                    throw JSONRPCError(RPC_DESERIALIZATION_ERROR, err);
                }
                Coin newcoin;
                newcoin.out.scriptPubKey = scriptPubKey;
                newcoin.out.SetReferenceValue(MAX_MONEY);
                if (prevOut.exists("value")) {
                    newcoin.out.SetReferenceValue(AmountFromValue(find_value(prevOut, "value")));
                }
                newcoin.nHeight = 1;
                newcoin.refheight = 0;
                if (prevOut.exists("refheight")) {
                    newcoin.refheight = find_value(prevOut, "refheight").get_int();
                }
                coins[out] = std::move(newcoin);
            }

            // if redeemScript and private keys were given, add redeemScript to the keystore so it can be signed
            const bool is_p2sh = scriptPubKey.IsPayToScriptHash();
            const bool is_p2wsh = scriptPubKey.IsPayToWitnessScriptHash();
            if (keystore && (is_p2sh || is_p2wsh)) {
                RPCTypeCheckObj(prevOut,
                    {
                        {"redeemScript", UniValueType(UniValue::VSTR)},
                        {"witnessScript", UniValueType(UniValue::VSTR)},
                    }, true);
                UniValue rs = find_value(prevOut, "redeemScript");
                UniValue ws = find_value(prevOut, "witnessScript");
                if (rs.isNull() && ws.isNull()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing redeemScript/witnessScript");
                }

                // work from witnessScript when possible
                CScript script;
                if (!ws.isNull()) {
                    std::vector<unsigned char> wsData = ParseHexV(ws, "witnessScript");
                    if (wsData.empty() || wsData[0] != 0x00) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unrecognized witnessScript version");
                    }
                    WitnessV0ScriptEntry entry(wsData);
                    keystore->AddWitnessV0Script(entry);
                    script = CScript(wsData.begin() + 1, wsData.end());
                    keystore->AddCScript(script);
                    keystore->AddCScript(GetScriptForWitness(script));
                    keystore->AddCScript(GetScriptForDestination(entry.GetLongHash()));
                    keystore->AddCScript(GetScriptForDestination(entry.GetShortHash()));
                } else if (!rs.isNull()) {
                    std::vector<unsigned char> rsData = ParseHexV(rs, "redeemScript");
                    script = CScript(rsData.begin(), rsData.end());
                    keystore->AddCScript(script);
                    // Automatically also add the P2WSH wrapped version of the
                    // script (to deal with P2SH-P2WSH).
                    // This is done for redeemScript only for compatibility, it
                    // is encouraged to use the explicit witnessScript field
                    // instead.
                    CScript witness_output_script{GetScriptForWitness(script)};
                    keystore->AddCScript(witness_output_script);
                    WitnessV0ScriptEntry entry(0 /* version */, script);
                    keystore->AddWitnessV0Script(entry);
                }

                if (!ws.isNull() && !rs.isNull()) {
                    // if both witnessScript and redeemScript are provided,
                    // they should either be the same (for backwards compat),
                    // or the redeemScript should be the encoded form of
                    // the witnessScript (ie, for p2sh-p2wsh)
                    std::vector<unsigned char> rsData = ParseHexV(rs, "redeemScript");
                    CScript redeem_script = CScript(rsData.begin(), rsData.end());
                    if (redeem_script != script) {
                        if (redeem_script != GetScriptForWitness(script)) {
                            throw JSONRPCError(RPC_INVALID_PARAMETER, "redeemScript does not correspond to witnessScript");
                        }
                    }
                }

                if (is_p2sh) {
                    const CTxDestination p2sh{ScriptHash(script)};
                    const CTxDestination p2sh_p2wsh{ScriptHash(GetScriptForWitness(script))};
                    if (scriptPubKey == GetScriptForDestination(p2sh)) {
                        // traditional p2sh; arguably an error if
                        // we got here with rs.IsNull(), because
                        // that means the p2sh script was specified
                        // via witnessScript param, but for now
                        // we'll just quietly accept it
                    } else if (scriptPubKey == GetScriptForDestination(p2sh_p2wsh)) {
                        // p2wsh encoded as p2sh; ideally the witness
                        // script was specified in the witnessScript
                        // param, but also support specifying it via
                        // redeemScript param for backwards compat
                        // (in which case ws.IsNull() == true)
                    } else {
                        // otherwise, can't generate scriptPubKey from
                        // either script, so we got unusable parameters
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "redeemScript/witnessScript does not match scriptPubKey");
                    }
                } else if (is_p2wsh) {
                    // plain p2wsh; could throw an error if script
                    // was specified by redeemScript rather than
                    // witnessScript (ie, ws.IsNull() == true), but
                    // accept it for backwards compat
                    const CTxDestination p2wsh{WitnessV0LongHash(0 /* version */, script)};
                    const CTxDestination p2wpk{WitnessV0ShortHash(0 /* version */, script)};
                    if (scriptPubKey != GetScriptForDestination(p2wsh) && scriptPubKey != GetScriptForDestination(p2wpk)) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "redeemScript/witnessScript does not match scriptPubKey");
                    }
                }
            }
        }
    }
}

UniValue SignTransaction(CMutableTransaction& mtx, const SigningProvider* keystore, const std::map<COutPoint, Coin>& coins, const UniValue& hashType)
{
    int nHashType = ParseSighashString(hashType);

    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Script verification errors
    UniValue vErrors(UniValue::VARR);

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mtx);
    // Sign what we can:
    for (unsigned int i = 0; i < mtx.vin.size(); i++) {
        CTxIn& txin = mtx.vin[i];
        auto coin = coins.find(txin.prevout);
        if (coin == coins.end() || coin->second.IsSpent()) {
            TxInErrorToJSON(txin, vErrors, "Input not found or already spent");
            continue;
        }
        const CScript& prevPubKey = coin->second.out.scriptPubKey;
        const CAmount& value = coin->second.out.GetReferenceValue();
        const int64_t refheight = coin->second.refheight;

        SignatureData sigdata = DataFromTransaction(mtx, i, coin->second.out, refheight);
        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mtx.vout.size())) {
            ProduceSignature(*keystore, MutableTransactionSignatureCreator(&mtx, i, value, refheight, nHashType), prevPubKey, sigdata);
        }

        UpdateInput(txin, sigdata);

        // value must be specified for valid segwit signature
        if (value == MAX_MONEY && !txin.scriptWitness.IsNull()) {
            throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing value for %s", coin->second.out.ToString()));
        }

        ScriptError serror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, i, value, refheight), &serror)) {
            if (serror == SCRIPT_ERR_INVALID_STACK_OPERATION) {
                // Unable to sign input and verification failed (possible attempt to partially sign).
                TxInErrorToJSON(txin, vErrors, "Unable to sign input, invalid stack size (possibly missing key)");
            } else if (serror == SCRIPT_ERR_NULLFAIL) {
                // Verification failed (possibly due to insufficient signatures).
                TxInErrorToJSON(txin, vErrors, "CHECK(MULTI)SIG failing with non-zero signature (possibly need more signatures)");
            } else {
                TxInErrorToJSON(txin, vErrors, ScriptErrorString(serror));
            }
        }
    }
    bool fComplete = vErrors.empty();

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(CTransaction(mtx)));
    result.pushKV("complete", fComplete);
    if (!vErrors.empty()) {
        result.pushKV("errors", vErrors);
    }

    return result;
}
