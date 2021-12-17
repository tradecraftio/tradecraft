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

#ifndef BITCOIN_ADDRDB_H
#define BITCOIN_ADDRDB_H

#include <fs.h>
#include <net_types.h> // For banmap_t
#include <serialize.h>

#include <string>
#include <map>

class CSubNet;
class CAddrMan;
class CDataStream;

class CBanEntry
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime;
    int64_t nBanUntil;

    CBanEntry()
    {
        SetNull();
    }

    explicit CBanEntry(int64_t nCreateTimeIn)
    {
        SetNull();
        nCreateTime = nCreateTimeIn;
    }

    SERIALIZE_METHODS(CBanEntry, obj)
    {
        uint8_t ban_reason = 2; //! For backward compatibility
        READWRITE(obj.nVersion, obj.nCreateTime, obj.nBanUntil, ban_reason);
    }

    void SetNull()
    {
        nVersion = CBanEntry::CURRENT_VERSION;
        nCreateTime = 0;
        nBanUntil = 0;
    }
};

/** Access to the (IP) address database (peers.dat) */
class CAddrDB
{
private:
    fs::path pathAddr;
public:
    CAddrDB();
    bool Write(const CAddrMan& addr);
    bool Read(CAddrMan& addr);
    static bool Read(CAddrMan& addr, CDataStream& ssPeers);
};

/** Access to the banlist database (banlist.dat) */
class CBanDB
{
private:
    const fs::path m_ban_list_path;
public:
    explicit CBanDB(fs::path ban_list_path);
    bool Write(const banmap_t& banSet);
    bool Read(banmap_t& banSet);
};

#endif // BITCOIN_ADDRDB_H
