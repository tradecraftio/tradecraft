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

#ifndef FREICOIN_PST_H
#define FREICOIN_PST_H

#include <attributes.h>
#include <node/transaction.h>
#include <optional.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/sign.h>
#include <script/signingprovider.h>

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

/** A structure for PSTs which contain per-input information */
struct PSTInput
{
    CTransactionRef non_witness_utxo;
    CTxOut witness_utxo;
    uint32_t witness_refheight;
    CScript redeem_script;
    WitnessV0ScriptEntry witness_entry;
    CScript final_script_sig;
    CScriptWitness final_script_witness;
    std::map<CPubKey, KeyOriginInfo> hd_keypaths;
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
            if (!witness_entry.IsNull()) {
                SerializeToVector(s, PST_IN_WITNESSSCRIPT);
                s << witness_entry;
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
                    if (!witness_entry.IsNull()) {
                        throw std::ios_base::failure("Duplicate Key, input witnessScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input witnessScript key is more than one byte type");
                    }
                    s >> witness_entry;
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
    WitnessV0ScriptEntry witness_entry;
    std::map<CPubKey, KeyOriginInfo> hd_keypaths;
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
        if (!witness_entry.IsNull()) {
            SerializeToVector(s, PST_OUT_WITNESSSCRIPT);
            s << witness_entry;
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
                    if (!witness_entry.IsNull()) {
                        throw std::ios_base::failure("Duplicate Key, output witnessScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output witnessScript key is more than one byte type");
                    }
                    s >> witness_entry;
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

    /** Merge pst into this. The two psts must have the same underlying CTransaction (i.e. the
      * same actual Freicoin transaction.) Returns true if the merge succeeded, false otherwise. */
    NODISCARD bool Merge(const PartiallySignedTransaction& pst);
    bool IsSane() const;
    bool AddInput(const CTxIn& txin, PSTInput& pstin);
    bool AddOutput(const CTxOut& txout, const PSTOutput& pstout);
    PartiallySignedTransaction() {}
    PartiallySignedTransaction(const PartiallySignedTransaction& pst_in) : tx(pst_in.tx), inputs(pst_in.inputs), outputs(pst_in.outputs), unknown(pst_in.unknown) {}
    explicit PartiallySignedTransaction(const CMutableTransaction& tx);
    /**
     * Finds the UTXO for a given input index
     *
     * @param[out] utxo The UTXO of the input if found
     * @param[out] refheight The reference height o the input if found
     * @param[in] input_index Index of the input to retrieve the UTXO of
     * @return Whether the UTXO for the specified input was found
     */
    bool GetInputUTXO(CTxOut& utxo, uint32_t& refheight, int input_index) const;

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

enum class PSTRole {
    CREATOR,
    UPDATER,
    SIGNER,
    FINALIZER,
    EXTRACTOR
};

std::string PSTRoleName(PSTRole role);

/** Checks whether a PSTInput is already signed. */
bool PSTInputSigned(const PSTInput& input);

/** Signs a PSTInput, verifying that all provided data matches what is being signed. */
bool SignPSTInput(const SigningProvider& provider, PartiallySignedTransaction& pst, int index, int sighash, SignatureData* out_sigdata = nullptr, bool use_dummy = false);

/** Updates a PSTOutput with information from provider.
 *
 * This fills in the redeem_script, witness_script, and hd_keypaths where possible.
 */
void UpdatePSTOutput(const SigningProvider& provider, PartiallySignedTransaction& pst, int index);

/**
 * Finalizes a PST if possible, combining partial signatures.
 *
 * @param[in,out] &pstx reference to PartiallySignedTransaction to finalize
 * return True if the PST is now complete, false otherwise
 */
bool FinalizePST(PartiallySignedTransaction& pstx);

/**
 * Finalizes a PST if possible, and extracts it to a CMutableTransaction if it could be finalized.
 *
 * @param[in]  &pstx reference to PartiallySignedTransaction
 * @param[out] result CMutableTransaction representing the complete transaction, if successful
 * @return True if we successfully extracted the transaction, false otherwise
 */
bool FinalizeAndExtractPST(PartiallySignedTransaction& pstx, CMutableTransaction& result);

/**
 * Combines PSTs with the same underlying transaction, resulting in a single PST with all partial signatures from each input.
 *
 * @param[out] &out   the combined PST, if successful
 * @param[in]  pstxs the PSTs to combine
 * @return error (OK if we successfully combined the transactions, other error if they were not compatible)
 */
NODISCARD TransactionError CombinePSTs(PartiallySignedTransaction& out, const std::vector<PartiallySignedTransaction>& pstxs);

//! Decode a hex PST into a PartiallySignedTransaction
NODISCARD bool DecodeHexPST(PartiallySignedTransaction& decoded_pst, const std::string& hex_pst, std::string& error);
//! Decode a raw (binary blob) PST into a PartiallySignedTransaction
NODISCARD bool DecodeRawPST(PartiallySignedTransaction& decoded_pst, const std::vector<unsigned char>& raw_pst, std::string& error);

#endif // FREICOIN_PST_H
