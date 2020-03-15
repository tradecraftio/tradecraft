// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include "keystore.h"

#include "consensus/merkle.h"
#include "key.h"
#include "pubkey.h"
#include "util.h"

#include <boost/foreach.hpp>

uint256 WitnessV0ScriptEntry::GetHash() const
{
    uint256 leaf;
    CHash256().Write(&m_script[0], m_script.size()).Finalize(leaf.begin());
    bool invalid = false;
    uint256 hash = ComputeFastMerkleRootFromBranch(leaf, m_branch, m_path, &invalid);
    if (invalid) {
        std::runtime_error(strprintf("%s: invalid Merkle proof", __func__));
    }
    return hash;
}

bool CKeyStore::AddKey(const CKey &key) {
    return AddKeyPubKey(key, key.GetPubKey());
}

bool CBasicKeyStore::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const
{
    CKey key;
    if (!GetKey(address, key)) {
        LOCK(cs_KeyStore);
        WatchKeyMap::const_iterator it = mapWatchKeys.find(address);
        if (it != mapWatchKeys.end()) {
            vchPubKeyOut = it->second;
            return true;
        }
        return false;
    }
    vchPubKeyOut = key.GetPubKey();
    return true;
}

bool CBasicKeyStore::AddKeyPubKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    return true;
}

bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
        return error("CBasicKeyStore::AddCScript(): redeemScripts > %i bytes are invalid", MAX_SCRIPT_ELEMENT_SIZE);

    LOCK(cs_KeyStore);
    mapScripts[CScriptID(redeemScript)] = redeemScript;
    return true;
}

bool CBasicKeyStore::HaveCScript(const CScriptID& hash) const
{
    LOCK(cs_KeyStore);
    return mapScripts.count(hash) > 0;
}

bool CBasicKeyStore::GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const
{
    LOCK(cs_KeyStore);
    ScriptMap::const_iterator mi = mapScripts.find(hash);
    if (mi != mapScripts.end())
    {
        redeemScriptOut = (*mi).second;
        return true;
    }
    return false;
}

bool CBasicKeyStore::AddWitnessV0Script(const WitnessV0ScriptEntry& entry)
{
    LOCK(cs_KeyStore);
    uint160 shorthash;
    CRIPEMD160().Write(entry.GetHash().begin(), 32).Finalize(shorthash.begin());
    mapWitnessV0Scripts[shorthash] = entry;
    return true;
}

bool CBasicKeyStore::HaveWitnessV0Script(const uint160& witnessprogram) const
{
    LOCK(cs_KeyStore);
    return mapWitnessV0Scripts.count(witnessprogram) > 0;
}

bool CBasicKeyStore::HaveWitnessV0Script(const uint256& witnessprogram) const
{
    uint160 shorthash;
    CRIPEMD160().Write(witnessprogram.begin(), 32).Finalize(shorthash.begin());
    return HaveWitnessV0Script(shorthash);
}

bool CBasicKeyStore::GetWitnessV0Script(const uint160& witnessprogram, WitnessV0ScriptEntry& entryOut) const
{
    LOCK(cs_KeyStore);
    auto mi = mapWitnessV0Scripts.find(witnessprogram);
    if (mi != mapWitnessV0Scripts.end())
    {
        entryOut = (*mi).second;
        return true;
    }
    return false;
}

bool CBasicKeyStore::GetWitnessV0Script(const uint256& witnessprogram, WitnessV0ScriptEntry& entryOut) const
{
    uint160 shorthash;
    CRIPEMD160().Write(witnessprogram.begin(), 32).Finalize(shorthash.begin());
    return GetWitnessV0Script(shorthash, entryOut);
}

static bool ExtractPubKey(const CScript &dest, CPubKey& pubKeyOut)
{
    //TODO: Use Solver to extract this?
    CScript::const_iterator pc = dest.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    if (!dest.GetOp(pc, opcode, vch) || vch.size() < 33 || vch.size() > 65)
        return false;
    pubKeyOut = CPubKey(vch);
    if (!pubKeyOut.IsFullyValid())
        return false;
    if (!dest.GetOp(pc, opcode, vch) || opcode != OP_CHECKSIG || dest.GetOp(pc, opcode, vch))
        return false;
    return true;
}

bool CBasicKeyStore::AddWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey))
        mapWatchKeys[pubKey.GetID()] = pubKey;
    return true;
}

bool CBasicKeyStore::RemoveWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.erase(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey))
        mapWatchKeys.erase(pubKey.GetID());
    return true;
}

bool CBasicKeyStore::HaveWatchOnly(const CScript &dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool CBasicKeyStore::HaveWatchOnly() const
{
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
}
