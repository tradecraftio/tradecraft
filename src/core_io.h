// Copyright (c) 2009-2016 The Bitcoin Core developers
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

#ifndef BITCOIN_CORE_IO_H
#define BITCOIN_CORE_IO_H

#include <string>
#include <vector>

class CBlock;
class CScript;
class CTransaction;
struct CMutableTransaction;
class uint256;
class UniValue;

// core_read.cpp
CScript ParseScript(const std::string& s);
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode = false);
bool DecodeHexTx(CMutableTransaction& tx, const std::string& strHexTx, bool fTryNoWitness = false);
bool DecodeHexBlk(CBlock&, const std::string& strHexBlk);
uint256 ParseHashUV(const UniValue& v, const std::string& strName);
uint256 ParseHashStr(const std::string&, const std::string& strName);
std::vector<unsigned char> ParseHexUV(const UniValue& v, const std::string& strName);

// core_write.cpp
std::string FormatScript(const CScript& script);
std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags = 0);
void ScriptPubKeyToUniv(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);
void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry);

#endif // BITCOIN_CORE_IO_H
