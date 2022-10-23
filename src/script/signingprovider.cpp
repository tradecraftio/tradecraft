// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
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

#include <script/keyorigin.h>
#include <script/signingprovider.h>
#include <script/standard.h>

#include <util/system.h>

const SigningProvider& DUMMY_SIGNING_PROVIDER = SigningProvider();

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

bool HidingSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const
{
    return m_provider->GetCScript(scriptid, script);
}

bool HidingSigningProvider::GetWitnessV0Script(const WitnessV0ShortHash& id, WitnessV0ScriptEntry& entry) const
{
    return m_provider->GetWitnessV0Script(id, entry);
}

bool HidingSigningProvider::GetWitnessV0Script(const WitnessV0LongHash& id, WitnessV0ScriptEntry& entry) const
{
    return GetWitnessV0Script(WitnessV0ShortHash(id), entry);
}

bool HidingSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const
{
    return m_provider->GetPubKey(keyid, pubkey);
}

bool HidingSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const
{
    if (m_hide_secret) return false;
    return m_provider->GetKey(keyid, key);
}

bool HidingSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
{
    if (m_hide_origin) return false;
    return m_provider->GetKeyOrigin(keyid, info);
}

bool FlatSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const { return LookupHelper(scripts, scriptid, script); }
bool FlatSigningProvider::GetWitnessV0Script(const WitnessV0ShortHash& id, WitnessV0ScriptEntry& entry) const { return LookupHelper(witscripts, id, entry); }
bool FlatSigningProvider::GetWitnessV0Script(const WitnessV0LongHash& id, WitnessV0ScriptEntry& entry) const { return GetWitnessV0Script(WitnessV0ShortHash(id), entry); }
bool FlatSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const { return LookupHelper(pubkeys, keyid, pubkey); }
bool FlatSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
{
    std::pair<CPubKey, KeyOriginInfo> out;
    bool ret = LookupHelper(origins, keyid, out);
    if (ret) info = std::move(out.second);
    return ret;
}
bool FlatSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const { return LookupHelper(keys, keyid, key); }

FlatSigningProvider& FlatSigningProvider::Merge(FlatSigningProvider&& b)
{
    scripts.merge(b.scripts);
    witscripts.merge(b.witscripts);
    pubkeys.merge(b.pubkeys);
    keys.merge(b.keys);
    origins.merge(b.origins);
    return *this;
}

void FillableSigningProvider::ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey)
{
    AssertLockHeld(cs_KeyStore);
    CKeyID key_id = pubkey.GetID();
    // This adds the redeemscripts necessary to detect P2WPK
    // outputs. Technically P2WPK outputs don't have a redeemscript to be
    // spent. However, our current IsMine logic requires the invlalid
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

bool FillableSigningProvider::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const
{
    CKey key;
    if (!GetKey(address, key)) {
        return false;
    }
    vchPubKeyOut = key.GetPubKey();
    return true;
}

bool FillableSigningProvider::AddKeyPubKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    ImplicitlyLearnRelatedKeyScripts(pubkey);
    return true;
}

bool FillableSigningProvider::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    return mapKeys.count(address) > 0;
}

std::set<CKeyID> FillableSigningProvider::GetKeys() const
{
    LOCK(cs_KeyStore);
    std::set<CKeyID> set_address;
    for (const auto& mi : mapKeys) {
        set_address.insert(mi.first);
    }
    return set_address;
}

bool FillableSigningProvider::GetKey(const CKeyID &address, CKey &keyOut) const
{
    LOCK(cs_KeyStore);
    KeyMap::const_iterator mi = mapKeys.find(address);
    if (mi != mapKeys.end()) {
        keyOut = mi->second;
        return true;
    }
    return false;
}

bool FillableSigningProvider::AddCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
        return error("FillableSigningProvider::AddCScript(): redeemScripts > %i bytes are invalid", MAX_SCRIPT_ELEMENT_SIZE);

    LOCK(cs_KeyStore);
    mapScripts[CScriptID(redeemScript)] = redeemScript;
    return true;
}

bool FillableSigningProvider::HaveCScript(const CScriptID& hash) const
{
    LOCK(cs_KeyStore);
    return mapScripts.count(hash) > 0;
}

std::set<CScriptID> FillableSigningProvider::GetCScripts() const
{
    LOCK(cs_KeyStore);
    std::set<CScriptID> set_script;
    for (const auto& mi : mapScripts) {
        set_script.insert(mi.first);
    }
    return set_script;
}

bool FillableSigningProvider::GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const
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

bool FillableSigningProvider::AddWitnessV0Script(const WitnessV0ScriptEntry& entry)
{
    LOCK(cs_KeyStore);
    mapWitnessV0Scripts[entry.GetShortHash()] = entry;
    return true;
}

bool FillableSigningProvider::HaveWitnessV0Script(const WitnessV0ShortHash& witnessprogram) const
{
    LOCK(cs_KeyStore);
    return mapWitnessV0Scripts.count(witnessprogram) > 0;
}

bool FillableSigningProvider::HaveWitnessV0Script(const WitnessV0LongHash& witnessprogram) const
{
    return HaveWitnessV0Script(WitnessV0ShortHash(witnessprogram));
}

std::set<WitnessV0ShortHash> FillableSigningProvider::GetWitnessV0Scripts() const
{
    LOCK(cs_KeyStore);
    std::set<WitnessV0ShortHash> ret;
    for (const auto& mi : mapWitnessV0Scripts) {
        ret.insert(mi.first);
    }
    return ret;
}

bool FillableSigningProvider::GetWitnessV0Script(const WitnessV0ShortHash& witnessprogram, WitnessV0ScriptEntry& entryOut) const
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

bool FillableSigningProvider::GetWitnessV0Script(const WitnessV0LongHash& witnessprogram, WitnessV0ScriptEntry& entryOut) const
{
    return GetWitnessV0Script(WitnessV0ShortHash(witnessprogram), entryOut);
}

static CKeyID GetKeyForWitnessV0Script(const SigningProvider& store, WitnessV0ScriptEntry& entry)
{
    if (!entry.m_script.empty() && entry.m_script[0] == 0x00) {
        CScript script(entry.m_script.begin() + 1, entry.m_script.end());
        CTxDestination dest;
        if (ExtractDestination(script, dest)) {
            if (auto id = std::get_if<PKHash>(&dest)) {
                return ToKeyID(*id);
            }
        }
    }

    return CKeyID();
}

CKeyID GetKeyForDestination(const SigningProvider& store, const CTxDestination& dest)
{
    // Only supports destinations which map to single public keys:
    // P2PKH, P2WPK, P2TR
    if (auto id = std::get_if<PKHash>(&dest)) {
        return ToKeyID(*id);
    }
    if (auto shortid = std::get_if<WitnessV0ShortHash>(&dest)) {
        WitnessV0ScriptEntry entry;
        if (store.GetWitnessV0Script(*shortid, entry)) {
            return GetKeyForWitnessV0Script(store, entry);
        }
    }
    if (auto longid = std::get_if<WitnessV0LongHash>(&dest)) {
        WitnessV0ScriptEntry entry;
        if (store.GetWitnessV0Script(*longid, entry)) {
            return GetKeyForWitnessV0Script(store, entry);
        }
    }
    if (auto script_hash = std::get_if<ScriptHash>(&dest)) {
        CScript script;
        CScriptID script_id(*script_hash);
        CTxDestination inner_dest;
        if (store.GetCScript(script_id, script) && ExtractDestination(script, inner_dest)) {
            if (auto id = std::get_if<PKHash>(&dest)) {
                return ToKeyID(*id);
            }
        }
    }
    return CKeyID();
}
