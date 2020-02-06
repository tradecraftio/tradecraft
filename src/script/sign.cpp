// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
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

#include <script/sign.h>

#include <key.h>
#include <keystore.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <uint256.h>

#include <algorithm>

typedef std::vector<unsigned char> valtype;

MutableTransactionSignatureCreator::MutableTransactionSignatureCreator(const CMutableTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int64_t refheightIn, int nHashTypeIn) : txTo(txToIn), nIn(nInIn), nHashType(nHashTypeIn), amount(amountIn), refheight(refheightIn), checker(txTo, nIn, amountIn, refheightIn) {}

bool MutableTransactionSignatureCreator::CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& address, const CScript& scriptCode, SigVersion sigversion) const
{
    CKey key;
    if (!provider.GetKey(address, key))
        return false;

    // Signing with uncompressed keys is disabled in witness scripts
    if (sigversion == SigVersion::WITNESS_V0 && !key.IsCompressed())
        return false;

    uint256 hash = SignatureHash(scriptCode, *txTo, nIn, nHashType, amount, refheight, sigversion);
    if (!key.Sign(hash, vchSig))
        return false;
    vchSig.push_back((unsigned char)nHashType);
    return true;
}

static bool GetCScript(const SigningProvider& provider, const SignatureData& sigdata, const CScriptID& scriptid, CScript& script)
{
    if (provider.GetCScript(scriptid, script)) {
        return true;
    }
    // Look for scripts in SignatureData
    if (CScriptID(sigdata.redeem_script) == scriptid) {
        script = sigdata.redeem_script;
        return true;
    }
    if (!sigdata.witness_entry.m_script.empty()) {
        CScript witness_script(sigdata.witness_entry.m_script.begin() + 1, sigdata.witness_entry.m_script.end());
        if (CScriptID(witness_script) == scriptid) {
            script = witness_script;
            return true;
        }
    }
    return false;
}

static bool GetWitnessV0Script(const SigningProvider& provider, const SignatureData& sigdata, const WitnessV0ShortHash& id, WitnessV0ScriptEntry& entry)
{
    if (provider.GetWitnessV0Script(id, entry)) {
        return true;
    }
    // Look for witscripts in SignatureData
    if (WitnessV0ShortHash((unsigned char)0, sigdata.redeem_script) == id) {
        entry.m_script.clear();
        entry.m_script.push_back(0x00);
        entry.m_script.insert(entry.m_script.end(), sigdata.redeem_script.begin(), sigdata.redeem_script.end());
        return true;
    }
    if (!sigdata.witness_entry.IsNull()) {
        if (sigdata.witness_entry.GetShortHash() == id) {
            entry = sigdata.witness_entry;
            return true;
        }
    }
    return false;
}

static bool GetWitnessV0Script(const SigningProvider& provider, const SignatureData& sigdata, const WitnessV0LongHash& id, WitnessV0ScriptEntry& entry) {
    return GetWitnessV0Script(provider, sigdata, WitnessV0ShortHash(id), entry);
}

static bool GetPubKey(const SigningProvider& provider, SignatureData& sigdata, const CKeyID& address, CPubKey& pubkey)
{
    if (provider.GetPubKey(address, pubkey)) {
        sigdata.misc_pubkeys.emplace(pubkey.GetID(), pubkey);
        return true;
    }
    // Look for pubkey in all partial sigs
    const auto it = sigdata.signatures.find(address);
    if (it != sigdata.signatures.end()) {
        pubkey = it->second.first;
        return true;
    }
    // Look for pubkey in pubkey list
    const auto& pk_it = sigdata.misc_pubkeys.find(address);
    if (pk_it != sigdata.misc_pubkeys.end()) {
        pubkey = pk_it->second;
        return true;
    }
    return false;
}

static bool CreateSig(const BaseSignatureCreator& creator, SignatureData& sigdata, const SigningProvider& provider, std::vector<unsigned char>& sig_out, const CPubKey& pubkey, const CScript& scriptcode, SigVersion sigversion)
{
    CKeyID keyid = pubkey.GetID();
    const auto it = sigdata.signatures.find(keyid);
    if (it != sigdata.signatures.end()) {
        sig_out = it->second.second;
        return true;
    }
    sigdata.misc_pubkeys.emplace(keyid, pubkey);
    if (creator.CreateSig(provider, sig_out, keyid, scriptcode, sigversion)) {
        auto i = sigdata.signatures.emplace(keyid, SigPair(pubkey, sig_out));
        assert(i.second);
        return true;
    }
    return false;
}

/**
 * Sign scriptPubKey using signature made with creator.
 * Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
 * unless whichTypeRet is TX_SCRIPTHASH, in which case scriptSigRet is the redemption script.
 * Returns false if scriptPubKey could not be completely satisfied.
 */
static bool SignStep(const SigningProvider& provider, const BaseSignatureCreator& creator, const CScript& scriptPubKey,
                     std::vector<valtype>& ret, txnouttype& whichTypeRet, SigVersion sigversion, SignatureData& sigdata)
{
    ret.clear();
    std::vector<unsigned char> sig;

    std::vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
        return false;

    switch (whichTypeRet)
    {
    case TX_NONSTANDARD:
    case TX_UNSPENDABLE:
    case TX_WITNESS_UNKNOWN:
        return false;
    case TX_PUBKEY:
        if (!CreateSig(creator, sigdata, provider, sig, CPubKey(vSolutions[0]), scriptPubKey, sigversion)) return false;
        ret.push_back(std::move(sig));
        return true;
    case TX_PUBKEYHASH: {
        CKeyID keyID = CKeyID(uint160(vSolutions[0]));
        CPubKey pubkey;
        GetPubKey(provider, sigdata, keyID, pubkey);
        if (!CreateSig(creator, sigdata, provider, sig, pubkey, scriptPubKey, sigversion)) return false;
        ret.push_back(std::move(sig));
        ret.push_back(ToByteVector(pubkey));
        return true;
    }
    case TX_SCRIPTHASH:
    {
        CScript scriptRet;
        if (GetCScript(provider, sigdata, uint160(vSolutions[0]), scriptRet)) {
            ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
            return true;
        }
        return false;
    }

    case TX_MULTISIG: {
        size_t required = vSolutions.front()[0];
        int nKeysCount = vSolutions.size() - 2;
        MultiSigHint hint(nKeysCount, (1 << nKeysCount) - 1);
        ret.push_back(valtype()); // hint
        for (size_t i = 1; i < vSolutions.size() - 1; ++i) {
            CPubKey pubkey = CPubKey(vSolutions[i]);
            if (ret.size() < required + 1 && CreateSig(creator, sigdata, provider, sig, pubkey, scriptPubKey, sigversion)) {
                hint.use_key(nKeysCount - i);
                ret.push_back(std::move(sig));
            }
        }
        ret[0] = hint.getvch();
        bool ok = ret.size() == required + 1;
        for (size_t i = 0; i + ret.size() < required + 1; ++i) {
            ret.push_back(valtype());
        }
        return ok;
    }
    case TX_WITNESS_V0_SHORTHASH:
    case TX_WITNESS_V0_LONGHASH:
    {
        bool found = false;
        WitnessV0ScriptEntry entry;
        if (whichTypeRet == TX_WITNESS_V0_SHORTHASH) {
            found = GetWitnessV0Script(provider, sigdata, WitnessV0ShortHash(vSolutions[0]), entry);
        }
        else if (whichTypeRet == TX_WITNESS_V0_LONGHASH) {
            found = GetWitnessV0Script(provider, sigdata, WitnessV0LongHash(vSolutions[0]), entry);
        }
        if (found) {
            if (!entry.m_script.empty() && (entry.m_script[0] == 0x00)) {
                // Return the WitnessV0ScriptEntry field in its entirety, so it
                // can be put into the SignatureData structure.
                CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                ss << entry;
                ret.push_back(ToByteVector(ss));
                return true;
            }
        }
        return false;
    }

    default:
        return false;
    }
}

static CScript PushAll(const std::vector<valtype>& values)
{
    CScript result;
    for (const valtype& v : values) {
        if (v.size() == 0) {
            result << OP_0;
        } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
            result << CScript::EncodeOP_N(v[0]);
        } else {
            result << v;
        }
    }
    return result;
}

bool ProduceSignature(const SigningProvider& provider, const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata)
{
    if (sigdata.complete) return true;

    std::vector<valtype> result;
    txnouttype whichType;
    bool solved = SignStep(provider, creator, fromPubKey, result, whichType, SigVersion::BASE, sigdata);
    bool P2SH = false;
    CScript subscript;
    sigdata.scriptWitness.stack.clear();

    if (solved && whichType == TX_SCRIPTHASH)
    {
        // Solver returns the subscript that needs to be evaluated;
        // the final scriptSig is the signatures from that
        // and then the serialized subscript:
        subscript = CScript(result[0].begin(), result[0].end());
        sigdata.redeem_script = subscript;
        solved = solved && SignStep(provider, creator, subscript, result, whichType, SigVersion::BASE, sigdata) && whichType != TX_SCRIPTHASH;
        P2SH = true;
    }

    if (solved && (whichType == TX_WITNESS_V0_SHORTHASH || whichType == TX_WITNESS_V0_LONGHASH))
    {
        CDataStream ss(result[0], SER_NETWORK, PROTOCOL_VERSION);
        ss >> sigdata.witness_entry;
        assert(ss.empty());
        CScript witnessscript(sigdata.witness_entry.m_script.begin() + 1, sigdata.witness_entry.m_script.end());
        txnouttype subType;
        solved = solved && SignStep(provider, creator, witnessscript, result, subType, SigVersion::WITNESS_V0, sigdata) && subType != TX_SCRIPTHASH && subType != TX_WITNESS_V0_LONGHASH && subType != TX_WITNESS_V0_SHORTHASH;
        // The second item on the stack (first to be pushed) is the witness script,
        // which is contained in the WitnessV0ScriptEntry passed back to us.
        result.push_back(sigdata.witness_entry.m_script);
        // The first item on the stack (last to be pushed) is the Merkle proof.
        // We construct the proof from the branch and path fields of the
        // WitnessV0ScriptEntry structure.
        result.emplace_back();
        // The path is specified in zero to four bytes in little endian order.
        // The exact number of bytes is implicit since the size of the field
        // which follows is known to be a multiple of 32.  So we add all the
        // bytes of path, and then remove any ending zero bytes, which are to be
        // understood implicitly.
        for (uint32_t path = sigdata.witness_entry.m_path; path; path >>= 8) {
            result.back().push_back((unsigned char)(path & 0xff));
        }
        // The branch hashes are serialized in order, without a length specifier
        // or padding bytes.
        for (const auto& hash : sigdata.witness_entry.m_branch) {
            result.back().insert(result.back().end(), hash.begin(), hash.end());
        }
        sigdata.scriptWitness.stack = result;
        sigdata.witness = true;
        result.clear();
    } else if (solved && whichType == TX_WITNESS_UNKNOWN) {
        sigdata.witness = true;
    }

    if (P2SH) {
        result.push_back(std::vector<unsigned char>(subscript.begin(), subscript.end()));
    }
    sigdata.scriptSig = PushAll(result);

    // Test solution
    sigdata.complete = solved && VerifyScript(sigdata.scriptSig, fromPubKey, &sigdata.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker());
    return sigdata.complete;
}

bool PSTInputSigned(PSTInput& input)
{
    return !input.final_script_sig.empty() || !input.final_script_witness.IsNull();
}

bool SignPSTInput(const SigningProvider& provider, PartiallySignedTransaction& pst, SignatureData& sigdata, int index, int sighash)
{
    PSTInput& input = pst.inputs.at(index);
    const CMutableTransaction& tx = *pst.tx;

    if (PSTInputSigned(input)) {
        return true;
    }

    // Fill SignatureData with input info
    input.FillSignatureData(sigdata);

    // Get UTXO
    bool require_witness_sig = false;
    CTxOut utxo;
    uint32_t refheight;

    // Verify input sanity, which checks that at most one of witness or non-witness utxos is provided.
    if (!input.IsSane()) {
        return false;
    }

    if (input.non_witness_utxo) {
        // If we're taking our information from a non-witness UTXO, verify that it matches the prevout.
        COutPoint prevout = tx.vin[index].prevout;
        if (input.non_witness_utxo->GetHash() != prevout.hash) {
            return false;
        }
        utxo = input.non_witness_utxo->vout[prevout.n];
        refheight = input.non_witness_utxo->lock_height;
    } else if (!input.witness_utxo.IsNull()) {
        utxo = input.witness_utxo;
        refheight = input.witness_refheight;
        // When we're taking our information from a witness UTXO, we can't verify it is actually data from
        // the output being spent. This is safe in case a witness signature is produced (which includes this
        // information directly in the hash), but not for non-witness signatures. Remember that we require
        // a witness signature in this situation.
        require_witness_sig = true;
    } else {
        return false;
    }

    MutableTransactionSignatureCreator creator(&tx, index, utxo.GetReferenceValue(), refheight, sighash);
    sigdata.witness = false;
    bool sig_complete = ProduceSignature(provider, creator, utxo.scriptPubKey, sigdata);
    // Verify that a witness signature was produced in case one was required.
    if (require_witness_sig && !sigdata.witness) return false;
    input.FromSignatureData(sigdata);

    // If we have a witness signature, use the smaller witness UTXO.
    if (sigdata.witness) {
        input.witness_utxo = utxo;
        input.witness_refheight = refheight;
        input.non_witness_utxo = nullptr;
    }

    return sig_complete;
}

class SignatureExtractorChecker final : public BaseSignatureChecker
{
private:
    SignatureData& sigdata;
    BaseSignatureChecker& checker;

public:
    SignatureExtractorChecker(SignatureData& sigdata, BaseSignatureChecker& checker) : sigdata(sigdata), checker(checker) {}
    bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
};

bool SignatureExtractorChecker::CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
{
    if (checker.CheckSig(scriptSig, vchPubKey, scriptCode, sigversion)) {
        CPubKey pubkey(vchPubKey);
        sigdata.signatures.emplace(pubkey.GetID(), SigPair(pubkey, scriptSig));
        return true;
    }
    return false;
}

namespace
{
struct Stacks
{
    std::vector<valtype> script;
    std::vector<valtype> witness;

    Stacks() = delete;
    Stacks(const Stacks&) = delete;
    explicit Stacks(const SignatureData& data) : witness(data.scriptWitness.stack) {
        EvalScript(script, data.scriptSig, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE);
    }
};
}

// Extracts signatures and scripts from incomplete scriptSigs. Please do not extend this, use PST instead
SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout, int64_t refheight)
{
    SignatureData data;
    assert(tx.vin.size() > nIn);
    data.scriptSig = tx.vin[nIn].scriptSig;
    data.scriptWitness = tx.vin[nIn].scriptWitness;
    Stacks stack(data);

    // Get signatures
    MutableTransactionSignatureChecker tx_checker(&tx, nIn, txout.GetReferenceValue(), refheight);
    SignatureExtractorChecker extractor_checker(data, tx_checker);
    if (VerifyScript(data.scriptSig, txout.scriptPubKey, &data.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, extractor_checker)) {
        data.complete = true;
        return data;
    }

    // Get scripts
    txnouttype script_type;
    std::vector<std::vector<unsigned char>> solutions;
    Solver(txout.scriptPubKey, script_type, solutions);
    SigVersion sigversion = SigVersion::BASE;
    CScript next_script = txout.scriptPubKey;

    if (script_type == TX_SCRIPTHASH && !stack.script.empty() && !stack.script.back().empty()) {
        // Get the redeemScript
        CScript redeem_script(stack.script.back().begin(), stack.script.back().end());
        data.redeem_script = redeem_script;
        next_script = std::move(redeem_script);

        // Get redeemScript type
        Solver(next_script, script_type, solutions);
        stack.script.pop_back();
    }
    if ((script_type == TX_WITNESS_V0_LONGHASH || script_type == TX_WITNESS_V0_SHORTHASH) && !(stack.witness.size() <= 1) && !(stack.witness.end() - 2)->empty() && ((stack.witness.end() - 2)->front() == 0x00)) {
        // Get the Merkle proof
        data.witness_entry.SetNull();
        size_t i = 0;
        for (; i < (stack.witness.back().size() % 32); ++i) {
            data.witness_entry.m_path <<= 8;
            data.witness_entry.m_path |= stack.witness.back()[i];
        }
        for (; i < stack.witness.back().size(); i += 32) {
            data.witness_entry.m_branch.emplace_back();
            std::copy(&stack.witness.back()[i], &stack.witness.back()[i + 32], data.witness_entry.m_branch.back().begin());
        }
        stack.witness.pop_back();

        // Get the witnessScript
        swap(data.witness_entry.m_script, stack.witness.back());
        stack.witness.pop_back();
        next_script = CScript(data.witness_entry.m_script.begin() + 1, data.witness_entry.m_script.end());

        // Get witnessScript type
        Solver(next_script, script_type, solutions);
        stack.script = std::move(stack.witness);
        stack.witness.clear();
        sigversion = SigVersion::WITNESS_V0;
    }
    if (script_type == TX_MULTISIG && !stack.script.empty()) {
        // Build a map of pubkey -> signature by matching sigs to pubkeys:
        assert(solutions.size() > 1);
        unsigned int num_pubkeys = solutions.size()-2;
        unsigned int last_success_key = 0;
        for (const valtype& sig : stack.script) {
            for (unsigned int i = last_success_key; i < num_pubkeys; ++i) {
                const valtype& pubkey = solutions[i+1];
                // We either have a signature for this pubkey, or we have found a signature and it is valid
                if (data.signatures.count(CPubKey(pubkey).GetID()) || extractor_checker.CheckSig(sig, pubkey, next_script, sigversion)) {
                    last_success_key = i + 1;
                    break;
                }
            }
        }
    }

    return data;
}

void UpdateInput(CTxIn& input, const SignatureData& data)
{
    input.scriptSig = data.scriptSig;
    input.scriptWitness = data.scriptWitness;
}

void SignatureData::MergeSignatureData(SignatureData sigdata)
{
    if (complete) return;
    if (sigdata.complete) {
        *this = std::move(sigdata);
        return;
    }
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    if (witness_entry.IsNull() && !sigdata.witness_entry.IsNull()) {
        witness_entry = sigdata.witness_entry;
    }
    signatures.insert(std::make_move_iterator(sigdata.signatures.begin()), std::make_move_iterator(sigdata.signatures.end()));
}

bool SignSignature(const SigningProvider &provider, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int64_t refheight, int nHashType)
{
    assert(nIn < txTo.vin.size());

    MutableTransactionSignatureCreator creator(&txTo, nIn, amount, refheight, nHashType);

    SignatureData sigdata;
    bool ret = ProduceSignature(provider, creator, fromPubKey, sigdata);
    UpdateInput(txTo.vin.at(nIn), sigdata);
    return ret;
}

bool SignSignature(const SigningProvider &provider, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType)
{
    assert(nIn < txTo.vin.size());
    CTxIn& txin = txTo.vin[nIn];
    assert(txin.prevout.n < txFrom.vout.size());
    const CTxOut& txout = txFrom.vout[txin.prevout.n];

    return SignSignature(provider, txout.scriptPubKey, txTo, nIn, txout.GetReferenceValue(), txFrom.lock_height, nHashType);
}

namespace {
/** Dummy signature checker which accepts all signatures. */
class DummySignatureChecker final : public BaseSignatureChecker
{
public:
    DummySignatureChecker() {}
    bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override { return true; }
};
const DummySignatureChecker DUMMY_CHECKER;

class DummySignatureCreator final : public BaseSignatureCreator {
private:
    char m_r_len = 32;
    char m_s_len = 32;
public:
    DummySignatureCreator(char r_len, char s_len) : m_r_len(r_len), m_s_len(s_len) {}
    const BaseSignatureChecker& Checker() const override { return DUMMY_CHECKER; }
    bool CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override
    {
        // Create a dummy signature that is a valid DER-encoding
        vchSig.assign(m_r_len + m_s_len + 7, '\000');
        vchSig[0] = 0x30;
        vchSig[1] = m_r_len + m_s_len + 4;
        vchSig[2] = 0x02;
        vchSig[3] = m_r_len;
        vchSig[4] = 0x01;
        vchSig[4 + m_r_len] = 0x02;
        vchSig[5 + m_r_len] = m_s_len;
        vchSig[6 + m_r_len] = 0x01;
        vchSig[6 + m_r_len + m_s_len] = SIGHASH_ALL;
        return true;
    }
};

template<typename M, typename K, typename V>
bool LookupHelper(const M& map, const K& key, V& value)
{
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        return true;
    }
    return false;
}

}

const BaseSignatureCreator& DUMMY_SIGNATURE_CREATOR = DummySignatureCreator(32, 32);
const BaseSignatureCreator& DUMMY_MAXIMUM_SIGNATURE_CREATOR = DummySignatureCreator(33, 32);
const SigningProvider& DUMMY_SIGNING_PROVIDER = SigningProvider();

bool IsSolvable(const SigningProvider& provider, const CScript& script)
{
    // This check is to make sure that the script we created can actually be solved for and signed by us
    // if we were to have the private keys. This is just to make sure that the script is valid and that,
    // if found in a transaction, we would still accept and relay that transaction. In particular,
    // it will reject witness outputs that require signing with an uncompressed public key.
    SignatureData sigs;
    // Make sure that STANDARD_SCRIPT_VERIFY_FLAGS includes SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, the most
    // important property this function is designed to test for.
    static_assert(STANDARD_SCRIPT_VERIFY_FLAGS & SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, "IsSolvable requires standard script flags to include WITNESS_PUBKEYTYPE");
    if (ProduceSignature(provider, DUMMY_SIGNATURE_CREATOR, script, sigs)) {
        // VerifyScript check is just defensive, and should never fail.
        assert(VerifyScript(sigs.scriptSig, script, &sigs.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, DUMMY_CHECKER));
        return true;
    }
    return false;
}

PartiallySignedTransaction::PartiallySignedTransaction(const CTransaction& tx) : tx(tx)
{
    inputs.resize(tx.vin.size());
    outputs.resize(tx.vout.size());
}

bool PartiallySignedTransaction::IsNull() const
{
    return !tx && inputs.empty() && outputs.empty() && unknown.empty();
}

void PartiallySignedTransaction::Merge(const PartiallySignedTransaction& pst)
{
    for (unsigned int i = 0; i < inputs.size(); ++i) {
        inputs[i].Merge(pst.inputs[i]);
    }
    for (unsigned int i = 0; i < outputs.size(); ++i) {
        outputs[i].Merge(pst.outputs[i]);
    }
    unknown.insert(pst.unknown.begin(), pst.unknown.end());
}

bool PartiallySignedTransaction::IsSane() const
{
    for (PSTInput input : inputs) {
        if (!input.IsSane()) return false;
    }
    return true;
}

bool PSTInput::IsNull() const
{
    return !non_witness_utxo && witness_utxo.IsNull() && partial_sigs.empty() && unknown.empty() && hd_keypaths.empty() && redeem_script.empty() && witness_entry.IsNull();
}

void PSTInput::FillSignatureData(SignatureData& sigdata) const
{
    if (!final_script_sig.empty()) {
        sigdata.scriptSig = final_script_sig;
        sigdata.complete = true;
    }
    if (!final_script_witness.IsNull()) {
        sigdata.scriptWitness = final_script_witness;
        sigdata.complete = true;
    }
    if (sigdata.complete) {
        return;
    }

    sigdata.signatures.insert(partial_sigs.begin(), partial_sigs.end());
    if (!redeem_script.empty()) {
        sigdata.redeem_script = redeem_script;
    }
    if (!witness_entry.IsNull()) {
        sigdata.witness_entry = witness_entry;
    }
    for (const auto& key_pair : hd_keypaths) {
        sigdata.misc_pubkeys.emplace(key_pair.first.GetID(), key_pair.first);
    }
}

void PSTInput::FromSignatureData(const SignatureData& sigdata)
{
    if (sigdata.complete) {
        partial_sigs.clear();
        hd_keypaths.clear();
        redeem_script.clear();
        witness_entry.SetNull();

        if (!sigdata.scriptSig.empty()) {
            final_script_sig = sigdata.scriptSig;
        }
        if (!sigdata.scriptWitness.IsNull()) {
            final_script_witness = sigdata.scriptWitness;
        }
        return;
    }

    partial_sigs.insert(sigdata.signatures.begin(), sigdata.signatures.end());
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    if (witness_entry.IsNull() && !sigdata.witness_entry.IsNull()) {
        witness_entry = sigdata.witness_entry;
    }
}

void PSTInput::Merge(const PSTInput& input)
{
    if (!non_witness_utxo && input.non_witness_utxo) non_witness_utxo = input.non_witness_utxo;
    if (witness_utxo.IsNull() && !input.witness_utxo.IsNull()) {
        witness_utxo = input.witness_utxo;
        witness_refheight = input.witness_refheight;
        non_witness_utxo = nullptr; // Clear out any non-witness utxo when we set a witness one.
    }

    partial_sigs.insert(input.partial_sigs.begin(), input.partial_sigs.end());
    hd_keypaths.insert(input.hd_keypaths.begin(), input.hd_keypaths.end());
    unknown.insert(input.unknown.begin(), input.unknown.end());

    if (redeem_script.empty() && !input.redeem_script.empty()) redeem_script = input.redeem_script;
    if (witness_entry.IsNull() && !input.witness_entry.IsNull()) witness_entry = input.witness_entry;
    if (final_script_sig.empty() && !input.final_script_sig.empty()) final_script_sig = input.final_script_sig;
    if (final_script_witness.IsNull() && !input.final_script_witness.IsNull()) final_script_witness = input.final_script_witness;
}

bool PSTInput::IsSane() const
{
    // Cannot have both witness and non-witness utxos
    if (!witness_utxo.IsNull() && non_witness_utxo) return false;

    // If we have a witness_entry or a scriptWitness, we must also have a witness utxo
    if (!witness_entry.IsNull() && witness_utxo.IsNull()) return false;
    if (!final_script_witness.IsNull() && witness_utxo.IsNull()) return false;

    return true;
}

void PSTOutput::FillSignatureData(SignatureData& sigdata) const
{
    if (!redeem_script.empty()) {
        sigdata.redeem_script = redeem_script;
    }
    if (!witness_entry.IsNull()) {
        sigdata.witness_entry = witness_entry;
    }
    for (const auto& key_pair : hd_keypaths) {
        sigdata.misc_pubkeys.emplace(key_pair.first.GetID(), key_pair.first);
    }
}

void PSTOutput::FromSignatureData(const SignatureData& sigdata)
{
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    if (witness_entry.IsNull() && !sigdata.witness_entry.IsNull()) {
        witness_entry = sigdata.witness_entry;
    }
}

bool PSTOutput::IsNull() const
{
    return redeem_script.empty() && witness_entry.IsNull() && hd_keypaths.empty() && unknown.empty();
}

void PSTOutput::Merge(const PSTOutput& output)
{
    hd_keypaths.insert(output.hd_keypaths.begin(), output.hd_keypaths.end());
    unknown.insert(output.unknown.begin(), output.unknown.end());

    if (redeem_script.empty() && !output.redeem_script.empty()) redeem_script = output.redeem_script;
    if (witness_entry.IsNull() && !output.witness_entry.IsNull()) witness_entry = output.witness_entry;
}

bool PublicOnlySigningProvider::GetCScript(const CScriptID &scriptid, CScript& script) const
{
    return m_provider->GetCScript(scriptid, script);
}

bool PublicOnlySigningProvider::GetWitnessV0Script(const WitnessV0ShortHash &id, WitnessV0ScriptEntry& entry) const
{
    return m_provider->GetWitnessV0Script(id, entry);
}

bool PublicOnlySigningProvider::GetPubKey(const CKeyID &address, CPubKey& pubkey) const
{
    return m_provider->GetPubKey(address, pubkey);
}

bool FlatSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const { return LookupHelper(scripts, scriptid, script); }
bool FlatSigningProvider::GetWitnessV0Script(const WitnessV0ShortHash &id, WitnessV0ScriptEntry& entry) const { return LookupHelper(witscripts, id, entry); }
bool FlatSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const { return LookupHelper(pubkeys, keyid, pubkey); }
bool FlatSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const { return LookupHelper(keys, keyid, key); }

FlatSigningProvider Merge(const FlatSigningProvider& a, const FlatSigningProvider& b)
{
    FlatSigningProvider ret;
    ret.scripts = a.scripts;
    ret.scripts.insert(b.scripts.begin(), b.scripts.end());
    ret.witscripts = a.witscripts;
    ret.witscripts.insert(b.witscripts.begin(), b.witscripts.end());
    ret.pubkeys = a.pubkeys;
    ret.pubkeys.insert(b.pubkeys.begin(), b.pubkeys.end());
    ret.keys = a.keys;
    ret.keys.insert(b.keys.begin(), b.keys.end());
    return ret;
}
