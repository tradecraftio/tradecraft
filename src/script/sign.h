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

#ifndef FREICOIN_SCRIPT_SIGN_H
#define FREICOIN_SCRIPT_SIGN_H

#include <boost/optional.hpp>
#include <hash.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/standard.h>
#include <streams.h>

class CKey;
class CKeyID;
class CScript;
class CScriptID;
class CTransaction;

struct CMutableTransaction;

/** An interface to be implemented by keystores that support signing. */
class SigningProvider
{
public:
    virtual ~SigningProvider() {}
    virtual bool GetCScript(const CScriptID &scriptid, CScript& script) const { return false; }
    virtual bool GetWitnessV0Script(const WitnessV0ScriptHash &id, std::vector<unsigned char>& witscript) const { return false; }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const { return false; }
    virtual bool GetKey(const CKeyID &address, CKey& key) const { return false; }
};

extern const SigningProvider& DUMMY_SIGNING_PROVIDER;

class PublicOnlySigningProvider : public SigningProvider
{
private:
    const SigningProvider* m_provider;

public:
    PublicOnlySigningProvider(const SigningProvider* provider) : m_provider(provider) {}
    bool GetCScript(const CScriptID &scriptid, CScript& script) const;
    bool GetWitnessV0Script(const WitnessV0ScriptHash &id, std::vector<unsigned char>& witscript) const;
    bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const;
};

struct FlatSigningProvider final : public SigningProvider
{
    std::map<CScriptID, CScript> scripts;
    std::map<WitnessV0ScriptHash, std::vector<unsigned char> > witscripts;
    std::map<CKeyID, CPubKey> pubkeys;
    std::map<CKeyID, CKey> keys;

    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetWitnessV0Script(const WitnessV0ScriptHash &id, std::vector<unsigned char>& witscript) const;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
};

FlatSigningProvider Merge(const FlatSigningProvider& a, const FlatSigningProvider& b);

/** Interface for signature creators. */
class BaseSignatureCreator {
public:
    virtual ~BaseSignatureCreator() {}
    virtual const BaseSignatureChecker& Checker() const =0;

    /** Create a singular (non-script) signature. */
    virtual bool CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const =0;
};

/** A signature creator for transactions. */
class MutableTransactionSignatureCreator : public BaseSignatureCreator {
    const CMutableTransaction* txTo;
    unsigned int nIn;
    int nHashType;
    CAmount amount;
    int64_t refheight;
    const MutableTransactionSignatureChecker checker;

public:
    MutableTransactionSignatureCreator(const CMutableTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int64_t refheightIn, int nHashTypeIn);
    const BaseSignatureChecker& Checker() const override { return checker; }
    bool CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override;
};

/** A signature creator that just produces 71-byte empty signatures. */
extern const BaseSignatureCreator& DUMMY_SIGNATURE_CREATOR;
/** A signature creator that just produces 72-byte empty signatures. */
extern const BaseSignatureCreator& DUMMY_MAXIMUM_SIGNATURE_CREATOR;

typedef std::pair<CPubKey, std::vector<unsigned char>> SigPair;

// This struct contains information from a transaction input and also contains signatures for that input.
// The information contained here can be used to create a signature and is also filled by ProduceSignature
// in order to construct final scriptSigs and scriptWitnesses.
struct SignatureData {
    bool complete = false; ///< Stores whether the scriptSig and scriptWitness are complete
    bool witness = false; ///< Stores whether the input this SigData corresponds to is a witness input
    CScript scriptSig; ///< The scriptSig of an input. Contains complete signatures or the traditional partial signatures format
    CScript redeem_script; ///< The redeemScript (if any) for the input
    CScript witness_script; ///< The witnessScript (if any) for the input. witnessScripts are used in P2WSH outputs.
    CScriptWitness scriptWitness; ///< The scriptWitness of an input. Contains complete signatures or the traditional partial signatures format. scriptWitness is part of a transaction input per BIP 144.
    std::map<CKeyID, SigPair> signatures; ///< BIP 174 style partial signatures for the input. May contain all signatures necessary for producing a final scriptSig or scriptWitness.
    std::map<CKeyID, CPubKey> misc_pubkeys;

    SignatureData() {}
    explicit SignatureData(const CScript& script) : scriptSig(script) {}
    void MergeSignatureData(SignatureData sigdata);
};

// Magic bytes
static constexpr uint8_t PST_MAGIC_BYTES[4] = {'p', 's', 't', 0xff};

// Global types
static constexpr uint8_t PST_GLOBAL_UNSIGNED_TX = 0x00;

// Input types
static constexpr uint8_t PST_IN_NON_WITNESS_UTXO = 0x00;
static constexpr uint8_t PST_IN_WITNESS_UTXO = 0x01;
static constexpr uint8_t PST_IN_PARTIAL_SIG = 0x02;
static constexpr uint8_t PST_IN_SIGHASH = 0x03;
static constexpr uint8_t PST_IN_REDEEMSCRIPT = 0x04;
static constexpr uint8_t PST_IN_WITNESSSCRIPT = 0x05;
static constexpr uint8_t PST_IN_BIP32_DERIVATION = 0x06;
static constexpr uint8_t PST_IN_SCRIPTSIG = 0x07;
static constexpr uint8_t PST_IN_SCRIPTWITNESS = 0x08;

// Output types
static constexpr uint8_t PST_OUT_REDEEMSCRIPT = 0x00;
static constexpr uint8_t PST_OUT_WITNESSSCRIPT = 0x01;
static constexpr uint8_t PST_OUT_BIP32_DERIVATION = 0x02;

// The separator is 0x00. Reading this in means that the unserializer can interpret it
// as a 0 length key which indicates that this is the separator. The separator has no value.
static constexpr uint8_t PST_SEPARATOR = 0x00;

// Takes a stream and multiple arguments and serializes them as if first serialized into a vector and then into the stream
// The resulting output into the stream has the total serialized length of all of the objects followed by all objects concatenated with each other.
template<typename Stream, typename... X>
void SerializeToVector(Stream& s, const X&... args)
{
    WriteCompactSize(s, GetSerializeSizeMany(s, args...));
    SerializeMany(s, args...);
}

// Takes a stream and multiple arguments and unserializes them first as a vector then each object individually in the order provided in the arguments
template<typename Stream, typename... X>
void UnserializeFromVector(Stream& s, X&... args)
{
    size_t expected_size = ReadCompactSize(s);
    size_t remaining_before = s.size();
    UnserializeMany(s, args...);
    size_t remaining_after = s.size();
    if (remaining_after + expected_size != remaining_before) {
        throw std::ios_base::failure("Size of value was not the stated size");
    }
}

// Deserialize HD keypaths into a map
template<typename Stream>
void DeserializeHDKeypaths(Stream& s, const std::vector<unsigned char>& key, std::map<CPubKey, std::vector<uint32_t>>& hd_keypaths)
{
    // Make sure that the key is the size of pubkey + 1
    if (key.size() != CPubKey::PUBLIC_KEY_SIZE + 1 && key.size() != CPubKey::COMPRESSED_PUBLIC_KEY_SIZE + 1) {
        throw std::ios_base::failure("Size of key was not the expected size for the type BIP32 keypath");
    }
    // Read in the pubkey from key
    CPubKey pubkey(key.begin() + 1, key.end());
    if (!pubkey.IsFullyValid()) {
       throw std::ios_base::failure("Invalid pubkey");
    }
    if (hd_keypaths.count(pubkey) > 0) {
        throw std::ios_base::failure("Duplicate Key, pubkey derivation path already provided");
    }

    // Read in key path
    uint64_t value_len = ReadCompactSize(s);
    std::vector<uint32_t> keypath;
    for (unsigned int i = 0; i < value_len; i += sizeof(uint32_t)) {
        uint32_t index;
        s >> index;
        keypath.push_back(index);
    }

    // Add to map
    hd_keypaths.emplace(pubkey, keypath);
}

// Serialize HD keypaths to a stream from a map
template<typename Stream>
void SerializeHDKeypaths(Stream& s, const std::map<CPubKey, std::vector<uint32_t>>& hd_keypaths, uint8_t type)
{
    for (auto keypath_pair : hd_keypaths) {
        if (!keypath_pair.first.IsValid()) {
            throw std::ios_base::failure("Invalid CPubKey being serialized");
        }
        SerializeToVector(s, type, MakeSpan(keypath_pair.first));
        WriteCompactSize(s, keypath_pair.second.size() * sizeof(uint32_t));
        for (auto& path : keypath_pair.second) {
            s << path;
        }
    }
}

/** A structure for PSTs which contain per-input information */
struct PSTInput
{
    CTransactionRef non_witness_utxo;
    CTxOut witness_utxo;
    uint32_t witness_refheight;
    CScript redeem_script;
    std::vector<unsigned char> witness_script;
    CScript final_script_sig;
    CScriptWitness final_script_witness;
    std::map<CPubKey, std::vector<uint32_t>> hd_keypaths;
    std::map<CKeyID, SigPair> partial_sigs;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;
    int sighash_type = 0;

    bool IsNull() const;
    void FillSignatureData(SignatureData& sigdata) const;
    void FromSignatureData(const SignatureData& sigdata);
    void Merge(const PSTInput& input);
    bool IsSane() const;
    PSTInput() {}

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        // Write the utxo
        // If there is a non-witness utxo, then don't add the witness one.
        if (non_witness_utxo) {
            SerializeToVector(s, PST_IN_NON_WITNESS_UTXO);
            OverrideStream<Stream> os(&s, s.GetType(), s.GetVersion() | SERIALIZE_TRANSACTION_NO_WITNESS);
            SerializeToVector(os, non_witness_utxo);
        } else if (!witness_utxo.IsNull()) {
            SerializeToVector(s, PST_IN_WITNESS_UTXO);
            SerializeToVector(s, witness_utxo, witness_refheight);
        }

        if (final_script_sig.empty() && final_script_witness.IsNull()) {
            // Write any partial signatures
            for (auto sig_pair : partial_sigs) {
                SerializeToVector(s, PST_IN_PARTIAL_SIG, MakeSpan(sig_pair.second.first));
                s << sig_pair.second.second;
            }

            // Write the sighash type
            if (sighash_type > 0) {
                SerializeToVector(s, PST_IN_SIGHASH);
                SerializeToVector(s, sighash_type);
            }

            // Write the redeem script
            if (!redeem_script.empty()) {
                SerializeToVector(s, PST_IN_REDEEMSCRIPT);
                s << redeem_script;
            }

            // Write the witness script
            if (!witness_script.empty()) {
                SerializeToVector(s, PST_IN_WITNESSSCRIPT);
                s << witness_script;
            }

            // Write any hd keypaths
            SerializeHDKeypaths(s, hd_keypaths, PST_IN_BIP32_DERIVATION);
        }

        // Write script sig
        if (!final_script_sig.empty()) {
            SerializeToVector(s, PST_IN_SCRIPTSIG);
            s << final_script_sig;
        }
        // write script witness
        if (!final_script_witness.IsNull()) {
            SerializeToVector(s, PST_IN_SCRIPTWITNESS);
            SerializeToVector(s, final_script_witness.stack);
        }

        // Write unknown things
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }

        s << PST_SEPARATOR;
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Read loop
        bool found_sep = false;
        while(!s.empty()) {
            // Read
            std::vector<unsigned char> key;
            s >> key;

            // the key is empty if that was actually a separator byte
            // This is a special case for key lengths 0 as those are not allowed (except for separator)
            if (key.empty()) {
                found_sep = true;
                break;
            }

            // First byte of key is the type
            unsigned char type = key[0];

            // Do stuff based on type
            switch(type) {
                case PST_IN_NON_WITNESS_UTXO:
                {
                    if (non_witness_utxo) {
                        throw std::ios_base::failure("Duplicate Key, input non-witness utxo already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Non-witness utxo key is more than one byte type");
                    }
                    // Set the stream to unserialize with witness since this is always a valid network transaction
                    OverrideStream<Stream> os(&s, s.GetType(), s.GetVersion() & ~SERIALIZE_TRANSACTION_NO_WITNESS);
                    UnserializeFromVector(os, non_witness_utxo);
                    break;
                }
                case PST_IN_WITNESS_UTXO:
                    if (!witness_utxo.IsNull()) {
                        throw std::ios_base::failure("Duplicate Key, input witness utxo already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Witness utxo key is more than one byte type");
                    }
                    UnserializeFromVector(s, witness_utxo, witness_refheight);
                    break;
                case PST_IN_PARTIAL_SIG:
                {
                    // Make sure that the key is the size of pubkey + 1
                    if (key.size() != CPubKey::PUBLIC_KEY_SIZE + 1 && key.size() != CPubKey::COMPRESSED_PUBLIC_KEY_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type partial signature pubkey");
                    }
                    // Read in the pubkey from key
                    CPubKey pubkey(key.begin() + 1, key.end());
                    if (!pubkey.IsFullyValid()) {
                       throw std::ios_base::failure("Invalid pubkey");
                    }
                    if (partial_sigs.count(pubkey.GetID()) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input partial signature for pubkey already provided");
                    }

                    // Read in the signature from value
                    std::vector<unsigned char> sig;
                    s >> sig;

                    // Add to list
                    partial_sigs.emplace(pubkey.GetID(), SigPair(pubkey, std::move(sig)));
                    break;
                }
                case PST_IN_SIGHASH:
                    if (sighash_type > 0) {
                        throw std::ios_base::failure("Duplicate Key, input sighash type already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Sighash type key is more than one byte type");
                    }
                    UnserializeFromVector(s, sighash_type);
                    break;
                case PST_IN_REDEEMSCRIPT:
                {
                    if (!redeem_script.empty()) {
                        throw std::ios_base::failure("Duplicate Key, input redeemScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input redeemScript key is more than one byte type");
                    }
                    s >> redeem_script;
                    break;
                }
                case PST_IN_WITNESSSCRIPT:
                {
                    if (!witness_script.empty()) {
                        throw std::ios_base::failure("Duplicate Key, input witnessScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input witnessScript key is more than one byte type");
                    }
                    s >> witness_script;
                    break;
                }
                case PST_IN_BIP32_DERIVATION:
                {
                    DeserializeHDKeypaths(s, key, hd_keypaths);
                    break;
                }
                case PST_IN_SCRIPTSIG:
                {
                    if (!final_script_sig.empty()) {
                        throw std::ios_base::failure("Duplicate Key, input final scriptSig already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Final scriptSig key is more than one byte type");
                    }
                    s >> final_script_sig;
                    break;
                }
                case PST_IN_SCRIPTWITNESS:
                {
                    if (!final_script_witness.IsNull()) {
                        throw std::ios_base::failure("Duplicate Key, input final scriptWitness already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Final scriptWitness key is more than one byte type");
                    }
                    UnserializeFromVector(s, final_script_witness.stack);
                    break;
                }
                // Unknown stuff
                default:
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    // Read in the value
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                    break;
            }
        }

        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of an input map");
        }
    }

    template <typename Stream>
    PSTInput(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};

/** A structure for PSTs which contains per output information */
struct PSTOutput
{
    CScript redeem_script;
    std::vector<unsigned char> witness_script;
    std::map<CPubKey, std::vector<uint32_t>> hd_keypaths;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;

    bool IsNull() const;
    void FillSignatureData(SignatureData& sigdata) const;
    void FromSignatureData(const SignatureData& sigdata);
    void Merge(const PSTOutput& output);
    bool IsSane() const;
    PSTOutput() {}

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        // Write the redeem script
        if (!redeem_script.empty()) {
            SerializeToVector(s, PST_OUT_REDEEMSCRIPT);
            s << redeem_script;
        }

        // Write the witness script
        if (!witness_script.empty()) {
            SerializeToVector(s, PST_OUT_WITNESSSCRIPT);
            s << witness_script;
        }

        // Write any hd keypaths
        SerializeHDKeypaths(s, hd_keypaths, PST_OUT_BIP32_DERIVATION);

        // Write unknown things
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }

        s << PST_SEPARATOR;
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Read loop
        bool found_sep = false;
        while(!s.empty()) {
            // Read
            std::vector<unsigned char> key;
            s >> key;

            // the key is empty if that was actually a separator byte
            // This is a special case for key lengths 0 as those are not allowed (except for separator)
            if (key.empty()) {
                found_sep = true;
                break;
            }

            // First byte of key is the type
            unsigned char type = key[0];

            // Do stuff based on type
            switch(type) {
                case PST_OUT_REDEEMSCRIPT:
                {
                    if (!redeem_script.empty()) {
                        throw std::ios_base::failure("Duplicate Key, output redeemScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output redeemScript key is more than one byte type");
                    }
                    s >> redeem_script;
                    break;
                }
                case PST_OUT_WITNESSSCRIPT:
                {
                    if (!witness_script.empty()) {
                        throw std::ios_base::failure("Duplicate Key, output witnessScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output witnessScript key is more than one byte type");
                    }
                    s >> witness_script;
                    break;
                }
                case PST_OUT_BIP32_DERIVATION:
                {
                    DeserializeHDKeypaths(s, key, hd_keypaths);
                    break;
                }
                // Unknown stuff
                default: {
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    // Read in the value
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                    break;
                }
            }
        }

        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of an output map");
        }
    }

    template <typename Stream>
    PSTOutput(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};

/** A version of CTransaction with the PST format*/
struct PartiallySignedTransaction
{
    boost::optional<CMutableTransaction> tx;
    std::vector<PSTInput> inputs;
    std::vector<PSTOutput> outputs;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;

    bool IsNull() const;
    void Merge(const PartiallySignedTransaction& pst);
    bool IsSane() const;
    PartiallySignedTransaction() {}
    PartiallySignedTransaction(const PartiallySignedTransaction& pst_in) : tx(pst_in.tx), inputs(pst_in.inputs), outputs(pst_in.outputs), unknown(pst_in.unknown) {}
    explicit PartiallySignedTransaction(const CTransaction& tx);

    // Only checks if they refer to the same transaction
    friend bool operator==(const PartiallySignedTransaction& a, const PartiallySignedTransaction &b)
    {
        return a.tx->GetHash() == b.tx->GetHash();
    }
    friend bool operator!=(const PartiallySignedTransaction& a, const PartiallySignedTransaction &b)
    {
        return !(a == b);
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const {

        // magic bytes
        s << PST_MAGIC_BYTES;

        // unsigned tx flag
        SerializeToVector(s, PST_GLOBAL_UNSIGNED_TX);

        // Write serialized tx to a stream
        OverrideStream<Stream> os(&s, s.GetType(), s.GetVersion() | SERIALIZE_TRANSACTION_NO_WITNESS);
        SerializeToVector(os, *tx);

        // Write the unknown things
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }

        // Separator
        s << PST_SEPARATOR;

        // Write inputs
        for (const PSTInput& input : inputs) {
            s << input;
        }
        // Write outputs
        for (const PSTOutput& output : outputs) {
            s << output;
        }
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Read the magic bytes
        uint8_t magic[4];
        s >> magic;
        if (!std::equal(magic, magic + 4, PST_MAGIC_BYTES)) {
            throw std::ios_base::failure("Invalid PST magic bytes");
        }

        // Read global data
        bool found_sep = false;
        while(!s.empty()) {
            // Read
            std::vector<unsigned char> key;
            s >> key;

            // the key is empty if that was actually a separator byte
            // This is a special case for key lengths 0 as those are not allowed (except for separator)
            if (key.empty()) {
                found_sep = true;
                break;
            }

            // First byte of key is the type
            unsigned char type = key[0];

            // Do stuff based on type
            switch(type) {
                case PST_GLOBAL_UNSIGNED_TX:
                {
                    if (tx) {
                        throw std::ios_base::failure("Duplicate Key, unsigned tx already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Global unsigned tx key is more than one byte type");
                    }
                    CMutableTransaction mtx;
                    // Set the stream to serialize with non-witness since this should always be non-witness
                    OverrideStream<Stream> os(&s, s.GetType(), s.GetVersion() | SERIALIZE_TRANSACTION_NO_WITNESS);
                    UnserializeFromVector(os, mtx);
                    tx = std::move(mtx);
                    // Make sure that all scriptSigs and scriptWitnesses are empty
                    for (const CTxIn& txin : tx->vin) {
                        if (!txin.scriptSig.empty() || !txin.scriptWitness.IsNull()) {
                            throw std::ios_base::failure("Unsigned tx does not have empty scriptSigs and scriptWitnesses.");
                        }
                    }
                    break;
                }
                // Unknown stuff
                default: {
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    // Read in the value
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                }
            }
        }

        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of the global map");
        }

        // Make sure that we got an unsigned tx
        if (!tx) {
            throw std::ios_base::failure("No unsigned transcation was provided");
        }

        // Read input data
        unsigned int i = 0;
        while (!s.empty() && i < tx->vin.size()) {
            PSTInput input;
            s >> input;
            inputs.push_back(input);

            // Make sure the non-witness utxo matches the outpoint
            if (input.non_witness_utxo && input.non_witness_utxo->GetHash() != tx->vin[i].prevout.hash) {
                throw std::ios_base::failure("Non-witness UTXO does not match outpoint hash");
            }
            ++i;
        }
        // Make sure that the number of inputs matches the number of inputs in the transaction
        if (inputs.size() != tx->vin.size()) {
            throw std::ios_base::failure("Inputs provided does not match the number of inputs in transaction.");
        }

        // Read output data
        i = 0;
        while (!s.empty() && i < tx->vout.size()) {
            PSTOutput output;
            s >> output;
            outputs.push_back(output);
            ++i;
        }
        // Make sure that the number of outputs matches the number of outputs in the transaction
        if (outputs.size() != tx->vout.size()) {
            throw std::ios_base::failure("Outputs provided does not match the number of outputs in transaction.");
        }
        // Sanity check
        if (!IsSane()) {
            throw std::ios_base::failure("PST is not sane.");
        }
    }

    template <typename Stream>
    PartiallySignedTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};

/** Produce a script signature using a generic signature creator. */
bool ProduceSignature(const SigningProvider& provider, const BaseSignatureCreator& creator, const CScript& scriptPubKey, SignatureData& sigdata);

/** Produce a script signature for a transaction. */
bool SignSignature(const SigningProvider &provider, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int64_t refheight, int nHashType);
bool SignSignature(const SigningProvider &provider, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType);

/** Checks whether a PSTInput is already signed. */
bool PSTInputSigned(PSTInput& input);

/** Signs a PSTInput, verifying that all provided data matches what is being signed. */
bool SignPSTInput(const SigningProvider& provider, PartiallySignedTransaction& pst, SignatureData& sigdata, int index, int sighash);

/** Extract signature data from a transaction input, and insert it. */
SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout, int64_t refheight);
void UpdateInput(CTxIn& input, const SignatureData& data);

/* Check whether we know how to sign for an output like this, assuming we
 * have all private keys. While this function does not need private keys, the passed
 * provider is used to look up public keys and redeemscripts by hash.
 * Solvability is unrelated to whether we consider this output to be ours. */
bool IsSolvable(const SigningProvider& provider, const CScript& script);

#endif // FREICOIN_SCRIPT_SIGN_H
