// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
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

#ifndef FREICOIN_KEYSTORE_H
#define FREICOIN_KEYSTORE_H

#include "key.h"
#include "pubkey.h"
#include "script/script.h"
#include "script/standard.h"
#include "sync.h"

#include <boost/signals2/signal.hpp>

/** Encapsulating class for information necessary to spend a witness
    output: the witness redeem script and Merkle proof. */
class WitnessV0ScriptEntry
{
public:
    std::vector<unsigned char> m_script;
    std::vector<uint256> m_branch;
    uint32_t m_path;

    WitnessV0ScriptEntry() : m_path(0) { }

    WitnessV0ScriptEntry(const std::vector<unsigned char>& scriptIn) : m_script(scriptIn), m_path(0) { }
    WitnessV0ScriptEntry(std::vector<unsigned char>&& scriptIn) : m_script(scriptIn), m_path(0) { }

    WitnessV0ScriptEntry(const std::vector<unsigned char>& scriptIn, const std::vector<uint256>& branchIn, uint32_t pathIn) : m_script(scriptIn), m_branch(branchIn), m_path(pathIn) { }
    WitnessV0ScriptEntry(std::vector<unsigned char>&& scriptIn, std::vector<uint256>&& branchIn, uint32_t pathIn) : m_script(scriptIn), m_branch(branchIn), m_path(pathIn) { }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(m_script);
        READWRITE(VARINT(m_path));
        READWRITE(m_branch);
    }

    void SetNull() {
        m_script.clear();
        m_branch.clear();
        m_path = 0;
    }

    uint256 GetHash() const;
};

/** A virtual base class for key stores */
class CKeyStore
{
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    //! Add a key to the store.
    virtual bool AddKeyPubKey(const CKey &key, const CPubKey &pubkey) =0;
    virtual bool AddKey(const CKey &key);

    //! Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(const CKeyID &address) const =0;
    virtual bool GetKey(const CKeyID &address, CKey& keyOut) const =0;
    virtual void GetKeys(std::set<CKeyID> &setAddress) const =0;
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const =0;

    //! Support for BIP 0013 : see https://github.com/bitcoin/bips/blob/master/bip-0013.mediawiki
    virtual bool AddCScript(const CScript& redeemScript) =0;
    virtual bool HaveCScript(const CScriptID &hash) const =0;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const =0;

    //! Support for witness scripts
    virtual bool AddWitnessV0Script(const WitnessV0ScriptEntry& entry) =0;
    virtual bool HaveWitnessV0Script(const uint160& witnesshash) const =0;
    virtual bool HaveWitnessV0Script(const uint256& witnesshash) const =0;
    virtual bool GetWitnessV0Script(const uint160& witnesshash, WitnessV0ScriptEntry& entryOut) const =0;
    virtual bool GetWitnessV0Script(const uint256& witnesshash, WitnessV0ScriptEntry& entryOut) const =0;

    //! Support for Watch-only addresses
    virtual bool AddWatchOnly(const CScript &dest) =0;
    virtual bool RemoveWatchOnly(const CScript &dest) =0;
    virtual bool HaveWatchOnly(const CScript &dest) const =0;
    virtual bool HaveWatchOnly() const =0;
};

typedef std::map<CKeyID, CKey> KeyMap;
typedef std::map<CKeyID, CPubKey> WatchKeyMap;
typedef std::map<CScriptID, CScript > ScriptMap;
typedef std::map<uint160, WitnessV0ScriptEntry> WitnessV0ScriptMap;
typedef std::set<CScript> WatchOnlySet;

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;
    WatchKeyMap mapWatchKeys;
    ScriptMap mapScripts;
    WitnessV0ScriptMap mapWitnessV0Scripts;
    WatchOnlySet setWatchOnly;

public:
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey) override;
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    bool HaveKey(const CKeyID &address) const override
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }
    void GetKeys(std::set<CKeyID> &setAddress) const override
    {
        setAddress.clear();
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.begin();
            while (mi != mapKeys.end())
            {
                setAddress.insert((*mi).first);
                mi++;
            }
        }
    }
    bool GetKey(const CKeyID &address, CKey &keyOut) const override
    {
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.find(address);
            if (mi != mapKeys.end())
            {
                keyOut = mi->second;
                return true;
            }
        }
        return false;
    }
    virtual bool AddCScript(const CScript& redeemScript) override;
    virtual bool HaveCScript(const CScriptID &hash) const override;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const override;

    virtual bool AddWitnessV0Script(const WitnessV0ScriptEntry& entry) override;
    virtual bool HaveWitnessV0Script(const uint160& witnessprogram) const override;
    virtual bool HaveWitnessV0Script(const uint256& witnessprogram) const override;
    virtual bool GetWitnessV0Script(const uint160& witnessprogram, WitnessV0ScriptEntry& entryOut) const override;
    virtual bool GetWitnessV0Script(const uint256& witnessprogram, WitnessV0ScriptEntry& entryOut) const override;

    virtual bool AddWatchOnly(const CScript &dest) override;
    virtual bool RemoveWatchOnly(const CScript &dest) override;
    virtual bool HaveWatchOnly(const CScript &dest) const override;
    virtual bool HaveWatchOnly() const override;
};

typedef std::vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;
typedef std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char> > > CryptedKeyMap;

#endif // FREICOIN_KEYSTORE_H
