// Copyright (c) 2009-2010 Satoshi Nakamoto
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

#ifndef FREICOIN_SCRIPT_SIGNINGPROVIDER_H
#define FREICOIN_SCRIPT_SIGNINGPROVIDER_H

#include <key.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>

struct KeyOriginInfo;

/** An interface to be implemented by keystores that support signing. */
class SigningProvider
{
public:
    virtual ~SigningProvider() {}
    virtual bool GetCScript(const CScriptID &scriptid, CScript& script) const { return false; }
    virtual bool HaveCScript(const CScriptID &scriptid) const { return false; }
    virtual bool HaveWitnessV0Script(const WitnessV0ScriptHash& witnesshash) const { return false; }
    virtual bool GetWitnessV0Script(const WitnessV0ScriptHash& witnesshash, WitnessV0ScriptEntry& entryOut) const { return false; }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const { return false; }
    virtual bool GetKey(const CKeyID &address, CKey& key) const { return false; }
    virtual bool HaveKey(const CKeyID &address) const { return false; }
    virtual bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const { return false; }
};

extern const SigningProvider& DUMMY_SIGNING_PROVIDER;

class HidingSigningProvider : public SigningProvider
{
private:
    const bool m_hide_secret;
    const bool m_hide_origin;
    const SigningProvider* m_provider;

public:
    HidingSigningProvider(const SigningProvider* provider, bool hide_secret, bool hide_origin) : m_hide_secret(hide_secret), m_hide_origin(hide_origin), m_provider(provider) {}
    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetWitnessV0Script(const WitnessV0ScriptHash& witnessprogram, WitnessV0ScriptEntry& entryOut) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
};

struct FlatSigningProvider final : public SigningProvider
{
    std::map<CScriptID, CScript> scripts;
    std::map<WitnessV0ScriptHash, WitnessV0ScriptEntry> witscripts;
    std::map<CKeyID, CPubKey> pubkeys;
    std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> origins;
    std::map<CKeyID, CKey> keys;

    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetWitnessV0Script(const WitnessV0ScriptHash& witnessprogram, WitnessV0ScriptEntry& entryOut) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
};

FlatSigningProvider Merge(const FlatSigningProvider& a, const FlatSigningProvider& b);

/** Fillable signing provider that keeps keys in an address->secret map */
class FillableSigningProvider : public SigningProvider
{
protected:
    mutable CCriticalSection cs_KeyStore;

    using KeyMap = std::map<CKeyID, CKey>;
    using ScriptMap = std::map<CScriptID, CScript>;
    using WitnessV0ScriptMap = std::map<WitnessV0ScriptHash, WitnessV0ScriptEntry>;

    KeyMap mapKeys GUARDED_BY(cs_KeyStore);
    ScriptMap mapScripts GUARDED_BY(cs_KeyStore);
    WitnessV0ScriptMap mapWitnessV0Scripts GUARDED_BY(cs_KeyStore);

    void ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

public:
    virtual bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    virtual bool AddKey(const CKey &key) { return AddKeyPubKey(key, key.GetPubKey()); }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    virtual bool HaveKey(const CKeyID &address) const override;
    virtual std::set<CKeyID> GetKeys() const;
    virtual bool GetKey(const CKeyID &address, CKey &keyOut) const override;
    virtual bool AddCScript(const CScript& redeemScript);
    virtual bool HaveCScript(const CScriptID &hash) const override;
    virtual std::set<CScriptID> GetCScripts() const;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const override;
    virtual bool AddWitnessV0Script(const WitnessV0ScriptEntry& entry);
    virtual bool HaveWitnessV0Script(const WitnessV0ScriptHash& witnessprogram) const override;
    virtual bool GetWitnessV0Script(const WitnessV0ScriptHash& witnessprogram, WitnessV0ScriptEntry& entryOut) const override;
};

/** Return the CKeyID of the key involved in a script (if there is a unique one). */
CKeyID GetKeyForDestination(const SigningProvider& store, const CTxDestination& dest);

#endif // FREICOIN_SCRIPT_SIGNINGPROVIDER_H
