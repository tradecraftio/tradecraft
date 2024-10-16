// Copyright (c) 2023 The Bitcoin Core developers
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

#include <addresstype.h>

#include <consensus/merkle.h> // for ComputeFastMerkleRootFromBranch
#include <crypto/sha256.h>
#include <hash.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/solver.h>
#include <uint256.h>
#include <util/hash_type.h>

#include <cassert>
#include <vector>

typedef std::vector<unsigned char> valtype;

ScriptHash::ScriptHash(const CScript& in) : BaseHash(Hash160(in)) {}
ScriptHash::ScriptHash(const CScriptID& in) : BaseHash{in} {}

PKHash::PKHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
PKHash::PKHash(const CKeyID& pubkey_id) : BaseHash(pubkey_id) {}

CKeyID ToKeyID(const PKHash& key_hash)
{
    return CKeyID{uint160{key_hash}};
}

CScriptID ToScriptID(const ScriptHash& script_hash)
{
    return CScriptID{uint160{script_hash}};
}

WitnessV0LongHash::WitnessV0LongHash(unsigned char version, const CScript& innerscript)
{
    CHash256().Write({&version, 1}).Write(innerscript).Finalize(m_hash);
}

WitnessV0ShortHash::WitnessV0ShortHash(unsigned char version, const CPubKey& pubkey) {
    assert(pubkey.IsCompressed());
    CScript p2pk = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
    WitnessV0LongHash longid(version, p2pk);
    CRIPEMD160().Write(longid.begin(), 32).Finalize(m_hash.begin());
}

WitnessV0ScriptEntry::WitnessV0ScriptEntry(unsigned char version, const CScript& innerscript) : m_path(0)
{
    m_script.push_back(version);
    m_script.insert(m_script.end(), innerscript.begin(), innerscript.end());
}

WitnessV0ScriptEntry::WitnessV0ScriptEntry(unsigned char version, const CScript& innerscript, const std::vector<uint256>& branchIn, uint32_t pathIn) : m_branch(branchIn), m_path(pathIn)
{
    m_script.push_back(version);
    m_script.insert(m_script.end(), innerscript.begin(), innerscript.end());
}

WitnessV0ScriptEntry::WitnessV0ScriptEntry(unsigned char version, const CScript& innerscript, std::vector<uint256>&& branchIn, uint32_t pathIn) : m_branch(branchIn), m_path(pathIn)
{
    m_script.push_back(version);
    m_script.insert(m_script.end(), innerscript.begin(), innerscript.end());
}

WitnessV0LongHash WitnessV0ScriptEntry::GetLongHash() const
{
    uint256 leaf;
    CHash256().Write(m_script).Finalize(leaf);
    bool invalid = false;
    uint256 hash = ComputeFastMerkleRootFromBranch(leaf, m_branch, m_path, &invalid);
    if (invalid) {
        throw std::runtime_error("invalid Merkle proof");
    }
    return WitnessV0LongHash(hash);
}

WitnessV0ShortHash WitnessV0ScriptEntry::GetShortHash() const
{
    return WitnessV0ShortHash(GetLongHash());
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    switch (whichType) {
    case TxoutType::PUBKEY: {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid()) {
            addressRet = CNoDestination(scriptPubKey);
        } else {
            addressRet = PubKeyDestination(pubKey);
        }
        return false;
    }
    case TxoutType::PUBKEYHASH: {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::SCRIPTHASH: {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::WITNESS_V0_SHORTHASH: {
        WitnessV0ShortHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_V0_LONGHASH: {
        WitnessV0LongHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_UNKNOWN: {
        addressRet = WitnessUnknown{vSolutions[0][0], vSolutions[1]};
        return true;
    }
    case TxoutType::MULTISIG:
    case TxoutType::NULL_DATA:
    case TxoutType::UNSPENDABLE:
    case TxoutType::NONSTANDARD:
        addressRet = CNoDestination(scriptPubKey);
        return false;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

namespace {
class CScriptVisitor
{
public:
    CScript operator()(const CNoDestination& dest) const
    {
        return dest.GetScript();
    }

    CScript operator()(const PubKeyDestination& dest) const
    {
        return CScript() << ToByteVector(dest.GetPubKey()) << OP_CHECKSIG;
    }

    CScript operator()(const PKHash& keyID) const
    {
        return CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
    }

    CScript operator()(const ScriptHash& scriptID) const
    {
        return CScript() << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
    }

    CScript operator()(const WitnessV0ShortHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }

    CScript operator()(const WitnessV0LongHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }

    CScript operator()(const WitnessUnknown& id) const
    {
        static const opcodetype versionmap[] = {
            /*  0 */OP_0,
            /*  1 */OP_1NEGATE,
            /*  2 */OP_1,
            /*  3 */OP_2,
            /*  4 */OP_3,
            /*  5 */OP_4,
            /*  6 */OP_5,
            /*  7 */OP_6,
            /*  8 */OP_7,
            /*  9 */OP_8,
            /* 10 */OP_9,
            /* 11 */OP_10,
            /* 12 */OP_11,
            /* 13 */OP_12,
            /* 14 */OP_13,
            /* 15 */OP_14,
            /* 16 */OP_15,
            /* 17 */OP_16,
            /* 18 */OP_NOP,
            /* 19 */OP_DEPTH,
            /* 20 */OP_CODESEPARATOR,
            /* 21 */OP_NOP1,
            /* 22 */OP_CHECKLOCKTIMEVERIFY,
            /* 23 */OP_CHECKSEQUENCEVERIFY,
            /* 24 */OP_MERKLEBRANCHVERIFY,
            /* 25 */OP_NOP5,
            /* 26 */OP_NOP6,
            /* 27 */OP_NOP7,
            /* 28 */OP_NOP8,
            /* 29 */OP_NOP9,
            /* 30 */OP_NOP10,
        };
        return CScript() << versionmap[id.GetWitnessVersion()] << id.GetWitnessProgram();
    }
};

class ValidDestinationVisitor
{
public:
    bool operator()(const CNoDestination& dest) const { return false; }
    bool operator()(const PubKeyDestination& dest) const { return false; }
    bool operator()(const PKHash& dest) const { return true; }
    bool operator()(const ScriptHash& dest) const { return true; }
    bool operator()(const WitnessV0ShortHash& dest) const { return true; }
    bool operator()(const WitnessV0LongHash& dest) const { return true; }
    bool operator()(const WitnessUnknown& dest) const { return true; }
};
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    return std::visit(CScriptVisitor(), dest);
}

bool IsValidDestination(const CTxDestination& dest) {
    return std::visit(ValidDestinationVisitor(), dest);
}
