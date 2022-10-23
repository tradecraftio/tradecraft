// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#ifndef FREICOIN_SCRIPT_STANDARD_H
#define FREICOIN_SCRIPT_STANDARD_H

#include <attributes.h>
#include <hash.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <uint256.h>
#include <util/hash_type.h>

#include <map>
#include <utility>

#include <string>
#include <variant>

static const bool DEFAULT_ACCEPT_DATACARRIER = false;

class CKeyID;
class CScript;
struct ScriptHash;

/** A reference to a CScript: the Hash160 of its serialization (see script.h) */
class CScriptID : public BaseHash<uint160>
{
public:
    CScriptID() : BaseHash() {}
    explicit CScriptID(const CScript& in);
    explicit CScriptID(const uint160& in) : BaseHash(in) {}
    explicit CScriptID(const ScriptHash& in);
};

/**
 * Default setting for -datacarriersize. 48 bytes of data, +1 for OP_RETURN,
 * +2 for the pushdata opcodes.
 */
static const unsigned int MAX_OP_RETURN_RELAY = 51;

/**
 * Mandatory script verification flags that all new blocks must comply with for
 * them to be valid. (but old blocks may not comply with) Currently just P2SH,
 * but in the future other flags may be added.
 *
 * Failing one of these tests may trigger a DoS ban - see CheckInputScripts() for
 * details.
 */
static const unsigned int MANDATORY_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_P2SH;

enum class TxoutType {
    NONSTANDARD,
    // 'standard' transaction types:
    PUBKEY,
    PUBKEYHASH,
    SCRIPTHASH,
    MULTISIG,
    NULL_DATA, //!< unspendable OP_RETURN script that carries data
    UNSPENDABLE, //!< unspendable, minimal (no-data) OP_RETURN script
    WITNESS_V0_LONGHASH,
    WITNESS_V0_SHORTHASH,
    WITNESS_UNKNOWN, //!< Only for Witness versions not already defined above
};

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
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
    unsigned int version;
    unsigned int length;
    unsigned char program[75];

    friend bool operator==(const WitnessUnknown& w1, const WitnessUnknown& w2) {
        if (w1.version != w2.version) return false;
        if (w1.length != w2.length) return false;
        return std::equal(w1.program, w1.program + w1.length, w2.program);
    }

    friend bool operator<(const WitnessUnknown& w1, const WitnessUnknown& w2) {
        if (w1.version < w2.version) return true;
        if (w1.version > w2.version) return false;
        if (w1.length < w2.length) return true;
        if (w1.length > w2.length) return false;
        return std::lexicographical_compare(w1.program, w1.program + w1.length, w2.program, w2.program + w2.length);
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
 * A txout script template with a specific destination. It is either:
 *  * CNoDestination: no destination set
 *  * PKHash: TxoutType::PUBKEYHASH destination (P2PKH)
 *  * ScriptHash: TxoutType::SCRIPTHASH destination (P2SH)
 *  * WitnessV0LongHash: TxoutType::WITNESS_V0_LONGHASH destination (P2WSH)
 *  * WitnessV0ShortHash: TxoutType::WITNESS_V0_SHORTHASH destination (P2WPK)
 *  * WitnessUnknown: TxoutType::WITNESS_UNKNOWN destination (P2W???)
 *  A CTxDestination is the internal data type encoded in a freicoin address
 */
using CTxDestination = std::variant<CNoDestination, PKHash, ScriptHash, WitnessV0LongHash, WitnessV0ShortHash, WitnessUnknown>;

/** Check whether a CTxDestination is a CNoDestination. */
bool IsValidDestination(const CTxDestination& dest);

/** Get the name of a TxoutType as a string */
std::string GetTxnOutputType(TxoutType t);

constexpr bool IsPushdataOp(opcodetype opcode)
{
    return opcode > OP_FALSE && opcode <= OP_PUSHDATA4;
}

/**
 * Parse a scriptPubKey and identify script type for standard scripts. If
 * successful, returns script type and parsed pubkeys or hashes, depending on
 * the type. For example, for a P2SH script, vSolutionsRet will contain the
 * script hash, for P2PKH it will contain the key hash, etc.
 *
 * @param[in]   scriptPubKey   Script to parse
 * @param[out]  vSolutionsRet  Vector of parsed pubkeys and hashes
 * @return                     The script type. TxoutType::NONSTANDARD represents a failed solve.
 */
TxoutType Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet);

/**
 * Parse a standard scriptPubKey for the destination address. Assigns result to
 * the addressRet parameter and returns true if successful. Currently only works for P2PK,
 * P2PKH, P2SH, P2WPK, and P2WSH scripts.
 */
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);

/**
 * Generate a Freicoin scriptPubKey for the given CTxDestination. Returns a P2PKH
 * script for a CKeyID destination, a P2SH script for a CScriptID, and an empty
 * script for CNoDestination.
 */
CScript GetScriptForDestination(const CTxDestination& dest);

/** Generate a P2PK script for the given pubkey. */
CScript GetScriptForRawPubKey(const CPubKey& pubkey);

/** Determine if script is a "multi_a" script. Returns (threshold, keyspans) if so, and nullopt otherwise.
 *  The keyspans refer to bytes in the passed script. */
std::optional<std::pair<int, std::vector<Span<const unsigned char>>>> MatchMultiA(const CScript& script LIFETIMEBOUND);

/** Generate a multisig script. */
CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys);

struct ShortestVectorFirstComparator
{
    bool operator()(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) const
    {
        if (a.size() < b.size()) return true;
        if (a.size() > b.size()) return false;
        return a < b;
    }
};

#endif // FREICOIN_SCRIPT_STANDARD_H
