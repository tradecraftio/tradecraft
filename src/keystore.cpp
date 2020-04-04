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

#include <keystore.h>

#include <util/system.h>

void CBasicKeyStore::ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey)
{
    AssertLockHeld(cs_KeyStore);
    CKeyID key_id = pubkey.GetID();
    // We must actually know about this key already.
    assert(HaveKey(key_id) || mapWatchKeys.count(key_id));
    // This adds the redeemscripts necessary to detect P2WPK and P2SH-P2WPK
    // outputs. Technically P2WPK outputs don't have a redeemscript to be
    // spent. However, our current IsMine logic requires the corresponding
    // P2SH-P2WPK redeemscript to be present in the wallet in order to accept
    // payment even to P2WPK outputs.
    // Also note that having superfluous scripts in the keystore never hurts.
    // They're only used to guide recursion in signing and IsMine logic - if
    // a script is present but we can't do anything with it, it has no effect.
    // "Implicitly" refers to fact that scripts are derived automatically from
    // existing keys, and are present in memory, even without being explicitly
    // loaded (e.g. from a file).
    if (pubkey.IsCompressed()) {
        CScript script = GetScriptForRawPubKey(pubkey);
        WitnessV0ScriptEntry entry(0 /* version */, script);
        // This does not use AddWitnessV0Script, as it may be overridden.
        mapWitnessV0Scripts[entry.GetShortHash()] = std::move(entry);
    }
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
    ImplicitlyLearnRelatedKeyScripts(pubkey);
    return true;
}

bool CBasicKeyStore::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    return mapKeys.count(address) > 0;
}

std::set<CKeyID> CBasicKeyStore::GetKeys() const
{
    LOCK(cs_KeyStore);
    std::set<CKeyID> set_address;
    for (const auto& mi : mapKeys) {
        set_address.insert(mi.first);
    }
    return set_address;
}

bool CBasicKeyStore::GetKey(const CKeyID &address, CKey &keyOut) const
{
    LOCK(cs_KeyStore);
    KeyMap::const_iterator mi = mapKeys.find(address);
    if (mi != mapKeys.end()) {
        keyOut = mi->second;
        return true;
    }
    return false;
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

std::set<CScriptID> CBasicKeyStore::GetCScripts() const
{
    LOCK(cs_KeyStore);
    std::set<CScriptID> set_script;
    for (const auto& mi : mapScripts) {
        set_script.insert(mi.first);
    }
    return set_script;
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
    mapWitnessV0Scripts[entry.GetShortHash()] = entry;
    return true;
}

bool CBasicKeyStore::HaveWitnessV0Script(const WitnessV0ShortHash& witnessprogram) const
{
    LOCK(cs_KeyStore);
    return mapWitnessV0Scripts.count(witnessprogram) > 0;
}

bool CBasicKeyStore::HaveWitnessV0Script(const WitnessV0LongHash& witnessprogram) const
{
    return HaveWitnessV0Script(WitnessV0ShortHash(witnessprogram));
}

std::set<WitnessV0ShortHash> CBasicKeyStore::GetWitnessV0Scripts() const
{
    LOCK(cs_KeyStore);
    std::set<WitnessV0ShortHash> ret;
    for (const auto& mi : mapWitnessV0Scripts) {
        ret.insert(mi.first);
    }
    return ret;
}

bool CBasicKeyStore::GetWitnessV0Script(const WitnessV0ShortHash& witnessprogram, WitnessV0ScriptEntry& entryOut) const
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

bool CBasicKeyStore::GetWitnessV0Script(const WitnessV0LongHash& witnessprogram, WitnessV0ScriptEntry& entryOut) const
{
    return GetWitnessV0Script(WitnessV0ShortHash(witnessprogram), entryOut);
}

static bool ExtractPubKey(const CScript &dest, CPubKey& pubKeyOut)
{
    //TODO: Use Solver to extract this?
    CScript::const_iterator pc = dest.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    if (!dest.GetOp(pc, opcode, vch) || !CPubKey::ValidSize(vch))
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
    if (ExtractPubKey(dest, pubKey)) {
        mapWatchKeys[pubKey.GetID()] = pubKey;
        ImplicitlyLearnRelatedKeyScripts(pubKey);
    }
    return true;
}

bool CBasicKeyStore::RemoveWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.erase(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        mapWatchKeys.erase(pubKey.GetID());
    }
    // Related CScripts are not removed; having superfluous scripts around is
    // harmless (see comment in ImplicitlyLearnRelatedKeyScripts).
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

static CKeyID GetKeyForWitnessV0Script(const CKeyStore& store, WitnessV0ScriptEntry& entry)
{
    if (!entry.m_script.empty() && entry.m_script[0] == 0x00) {
        CScript script(entry.m_script.begin() + 1, entry.m_script.end());
        CTxDestination dest;
        if (ExtractDestination(script, dest)) {
            if (auto id = boost::get<CKeyID>(&dest)) {
                return *id;
            }
        }
    }

    return CKeyID();
}

CKeyID GetKeyForDestination(const CKeyStore& store, const CTxDestination& dest)
{
    // Only supports destinations which map to single public keys, i.e. P2PKH,
    // P2WPKH, and P2SH-P2WPKH.
    if (auto id = boost::get<CKeyID>(&dest)) {
        return *id;
    }
    if (auto shortid = boost::get<WitnessV0ShortHash>(&dest)) {
        WitnessV0ScriptEntry entry;
        if (store.GetWitnessV0Script(*shortid, entry)) {
            return GetKeyForWitnessV0Script(store, entry);
        }
    }
    if (auto longid = boost::get<WitnessV0LongHash>(&dest)) {
        WitnessV0ScriptEntry entry;
        if (store.GetWitnessV0Script(*longid, entry)) {
            return GetKeyForWitnessV0Script(store, entry);
        }
    }
    if (auto script_id = boost::get<CScriptID>(&dest)) {
        CScript script;
        CTxDestination inner_dest;
        if (store.GetCScript(*script_id, script) && ExtractDestination(script, inner_dest)) {
            if (auto shortid = boost::get<WitnessV0ShortHash>(&inner_dest)) {
                WitnessV0ScriptEntry entry;
                if (store.GetWitnessV0Script(*shortid, entry)) {
                    return GetKeyForWitnessV0Script(store, entry);
                }
            }
            if (auto longid = boost::get<WitnessV0LongHash>(&inner_dest)) {
                WitnessV0ScriptEntry entry;
                if (store.GetWitnessV0Script(*longid, entry)) {
                    return GetKeyForWitnessV0Script(store, entry);
                }
            }
        }
    }
    return CKeyID();
}

bool HaveKey(const CKeyStore& store, const CKey& key)
{
    CKey key2;
    key2.Set(key.begin(), key.end(), !key.IsCompressed());
    return store.HaveKey(key.GetPubKey().GetID()) || store.HaveKey(key2.GetPubKey().GetID());
}
