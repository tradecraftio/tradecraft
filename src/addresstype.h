// Copyright (c) 2023 The Freicoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef FREICOIN_ADDRESSTYPE_H
#define FREICOIN_ADDRESSTYPE_H

#include <pubkey.h>
#include <hash.h>
#include <script/script.h>
#include <uint256.h>
#include <util/hash_type.h>

#include <variant>
#include <algorithm>
#include <utility> // for std::tie

class CNoDestination
{
private:
    CScript m_script;

public:
    CNoDestination() = default;
    explicit CNoDestination(const CScript& script) : m_script(script) {}

    const CScript& GetScript() const LIFETIMEBOUND { return m_script; }

    friend bool operator==(const CNoDestination& a, const CNoDestination& b) { return a.GetScript() == b.GetScript(); }
    friend bool operator<(const CNoDestination& a, const CNoDestination& b) { return a.GetScript() < b.GetScript(); }
};

struct PubKeyDestination {
private:
    CPubKey m_pubkey;

public:
    explicit PubKeyDestination(const CPubKey& pubkey) : m_pubkey(pubkey) {}

    const CPubKey& GetPubKey() const LIFETIMEBOUND { return m_pubkey; }

    friend bool operator==(const PubKeyDestination& a, const PubKeyDestination& b) { return a.GetPubKey() == b.GetPubKey(); }
    friend bool operator<(const PubKeyDestination& a, const PubKeyDestination& b) { return a.GetPubKey() < b.GetPubKey(); }
};

struct PKHash : public BaseHash<uint160>
{
    PKHash() : BaseHash() {}
    explicit PKHash(const uint160& hash) : BaseHash(hash) {}
    explicit PKHash(const CPubKey& pubkey);
    explicit PKHash(const CKeyID& pubkey_id);
};
CKeyID ToKeyID(const PKHash& key_hash);


struct ScriptHash : public BaseHash<uint160>
{
    ScriptHash() : BaseHash() {}
    // These don't do what you'd expect.
    // Use ScriptHash(GetScriptForDestination(...)) instead.
    explicit ScriptHash(const PKHash& hash) = delete;

    explicit ScriptHash(const uint160& hash) : BaseHash(hash) {}
    explicit ScriptHash(const CScript& script);
    explicit ScriptHash(const CScriptID& script);
};
CScriptID ToScriptID(const ScriptHash& script_hash);

struct WitnessV0LongHash : public BaseHash<uint256>
{
    WitnessV0LongHash() : BaseHash() {}
    explicit WitnessV0LongHash(const uint256& hash) : BaseHash(hash) {}
    WitnessV0LongHash(unsigned char version, const CScript& innerscript);
};

struct WitnessV0ShortHash : public BaseHash<uint160>
{
    WitnessV0ShortHash() : BaseHash() {}
    explicit WitnessV0ShortHash(const uint160& hash) : BaseHash(hash) {}
    explicit WitnessV0ShortHash(const WitnessV0LongHash &longid) {
        CRIPEMD160().Write(longid.begin(), 32).Finalize(begin());
    }
    WitnessV0ShortHash(unsigned char version, const CScript& innerscript) {
        WitnessV0LongHash longid(version, innerscript);
        CRIPEMD160().Write(longid.begin(), 32).Finalize(begin());
    }
    WitnessV0ShortHash(unsigned char version, const CPubKey& pubkey);
};

//! CTxDestination subtype to encode any future Witness version
struct WitnessUnknown
{
private:
    unsigned int m_version;
    std::vector<unsigned char> m_program;

public:
    WitnessUnknown(unsigned int version, const std::vector<unsigned char>& program) : m_version(version), m_program(program) {}
    WitnessUnknown(int version, const std::vector<unsigned char>& program) : m_version(static_cast<unsigned int>(version)), m_program(program) {}

    unsigned int GetWitnessVersion() const { return m_version; }
    const std::vector<unsigned char>& GetWitnessProgram() const LIFETIMEBOUND { return m_program; }

    friend bool operator==(const WitnessUnknown& w1, const WitnessUnknown& w2) {
        if (w1.GetWitnessVersion() != w2.GetWitnessVersion()) return false;
        return w1.GetWitnessProgram() == w2.GetWitnessProgram();
    }

    friend bool operator<(const WitnessUnknown& w1, const WitnessUnknown& w2) {
        if (w1.GetWitnessVersion() < w2.GetWitnessVersion()) return true;
        if (w1.GetWitnessVersion() > w2.GetWitnessVersion()) return false;
        return w1.GetWitnessProgram() < w2.GetWitnessProgram();
    }
};

/** Encapsulating class for information necessary to spend a witness
    output: the witness redeem script and Merkle proof. */
class WitnessV0ScriptEntry
{
public:
    std::vector<unsigned char> m_script;
    std::vector<uint256> m_branch;
    uint32_t m_path;

    WitnessV0ScriptEntry() : m_path(0) { }

    explicit WitnessV0ScriptEntry(const std::vector<unsigned char>& scriptIn) : m_script(scriptIn), m_path(0) { }
    explicit WitnessV0ScriptEntry(std::vector<unsigned char>&& scriptIn) : m_script(scriptIn), m_path(0) { }
    WitnessV0ScriptEntry(const std::vector<unsigned char>& scriptIn, const std::vector<uint256>& branchIn, uint32_t pathIn) : m_script(scriptIn), m_branch(branchIn), m_path(pathIn) { }
    WitnessV0ScriptEntry(std::vector<unsigned char>&& scriptIn, std::vector<uint256>&& branchIn, uint32_t pathIn) : m_script(scriptIn), m_branch(branchIn), m_path(pathIn) { }

    WitnessV0ScriptEntry(unsigned char version, const CScript& innerscript);
    WitnessV0ScriptEntry(unsigned char version, const CScript& innerscript, const std::vector<uint256>& branchIn, uint32_t pathIn);
    WitnessV0ScriptEntry(unsigned char version, const CScript& innerscript, std::vector<uint256>&& branchIn, uint32_t pathIn);

    SERIALIZE_METHODS(WitnessV0ScriptEntry, obj) {
        READWRITE(obj.m_script, VARINT(obj.m_path), obj.m_branch);
    }

    inline void SetNull() {
        m_script.clear();
        m_branch.clear();
        m_path = 0;
    }

    inline bool IsNull() const {
        return m_script.empty();
    }

    inline bool operator<(const WitnessV0ScriptEntry& rhs) const {
        return std::tie(m_script, m_branch, m_path) < std::tie(rhs.m_script, rhs.m_branch, rhs.m_path);
    }

    WitnessV0LongHash GetLongHash() const;
    WitnessV0ShortHash GetShortHash() const;
};

inline void swap(WitnessV0ScriptEntry& lhs, WitnessV0ScriptEntry& rhs) noexcept {
    using std::swap;
    swap(lhs.m_script, rhs.m_script);
    swap(lhs.m_branch, rhs.m_branch);
    swap(lhs.m_path, rhs.m_path);
}

/**
 * A txout script categorized into standard templates.
 *  * CNoDestination: Optionally a script, no corresponding address.
 *  * PubKeyDestination: TxoutType::PUBKEY (P2PK), no corresponding address
 *  * PKHash: TxoutType::PUBKEYHASH destination (P2PKH address)
 *  * ScriptHash: TxoutType::SCRIPTHASH destination (P2SH address)
 *  * WitnessV0LongHash: TxoutType::WITNESS_V0_LONGHASH destination (P2WSH address)
 *  * WitnessV0ShortHash: TxoutType::WITNESS_V0_SHORTHASH destination (P2WPK address)
 *  * WitnessUnknown: TxoutType::WITNESS_UNKNOWN destination (P2W??? address)
 *  A CTxDestination is the internal data type encoded in a freicoin address
 */
using CTxDestination = std::variant<CNoDestination, PubKeyDestination, PKHash, ScriptHash, WitnessV0LongHash, WitnessV0ShortHash, WitnessUnknown>;

/** Check whether a CTxDestination corresponds to one with an address. */
bool IsValidDestination(const CTxDestination& dest);

/**
 * Parse a scriptPubKey for the destination.
 *
 * For standard scripts that have addresses (and P2PK as an exception), a corresponding CTxDestination
 * is assigned to addressRet.
 * For all other scripts. addressRet is assigned as a CNoDestination containing the scriptPubKey.
 *
 * Returns true for standard destinations with addresses - P2PKH, P2SH, P2WPK, P2WSH, P2TR and P2W??? scripts.
 * Returns false for non-standard destinations and those without addresses - P2PK, bare multisig, null data, and nonstandard scripts.
 */
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);

/**
 * Generate a Freicoin scriptPubKey for the given CTxDestination. Returns a P2PKH
 * script for a CKeyID destination, a P2SH script for a CScriptID, and an empty
 * script for CNoDestination.
 */
CScript GetScriptForDestination(const CTxDestination& dest);

#endif // FREICOIN_ADDRESSTYPE_H
