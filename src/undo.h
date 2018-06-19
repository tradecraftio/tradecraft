// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#ifndef FREICOIN_UNDO_H
#define FREICOIN_UNDO_H

#include "compressor.h" 
#include "primitives/transaction.h"
#include "serialize.h"

/** Undo information for a CTxIn
 *
 *  Contains the prevout's CTxOut being spent, and if this was the
 *  last output of the affected transaction, its metadata as well
 *  (coinbase or not, height, transaction version)
 */
class CTxInUndo
{
public:
    CTxOut txout;         // the txout data before being spent
    bool fCoinBase;       // if the outpoint was the last unspent: whether it belonged to a coinbase
    unsigned int nHeight; // if the outpoint was the last unspent: its height
    int nVersion;         // if the outpoint was the last unspent: its version
    int refheight;        // if the outpoint was the last unspent: its refheight

    CTxInUndo() : txout(), fCoinBase(false), nHeight(0), nVersion(0), refheight(0) {}
    CTxInUndo(const CTxOut &txoutIn, bool fCoinBaseIn = false, unsigned int nHeightIn = 0, int nVersionIn = 0, int refheightIn = 0) : txout(txoutIn), fCoinBase(fCoinBaseIn), nHeight(nHeightIn), nVersion(nVersionIn), refheight(refheightIn) { }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        return ::GetSerializeSize(VARINT(nHeight*2+(fCoinBase ? 1 : 0)), nType, nVersion) +
               (nHeight > 0 ? ::GetSerializeSize(VARINT(this->nVersion), nType, nVersion) : 0) +
               ::GetSerializeSize(CTxOutCompressor(REF(txout)), nType, nVersion) +
               (nHeight > 0 ? ::GetSerializeSize(VARINT(refheight), nType, nVersion) : 0);
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        ::Serialize(s, VARINT(nHeight*2+(fCoinBase ? 1 : 0)), nType, nVersion);
        if (nHeight > 0)
            ::Serialize(s, VARINT(this->nVersion), nType, nVersion);
        ::Serialize(s, CTxOutCompressor(REF(txout)), nType, nVersion);
        if (nHeight > 0)
            ::Serialize(s, VARINT(refheight), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int nCode = 0;
        ::Unserialize(s, VARINT(nCode), nType, nVersion);
        nHeight = nCode / 2;
        fCoinBase = nCode & 1;
        if (nHeight > 0)
            ::Unserialize(s, VARINT(this->nVersion), nType, nVersion);
        ::Unserialize(s, REF(CTxOutCompressor(REF(txout))), nType, nVersion);
        if (nHeight > 0)
            ::Unserialize(s, VARINT(refheight), nType, nVersion);
    }
};

/** Undo information for a CTransaction */
class CTxUndo
{
public:
    // undo information for all txins
    std::vector<CTxInUndo> vprevout;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vprevout);
    }
};

/** Extended serialization of block undo structure, inclusive of the prior
 ** block-final transaction hash. Basically we have two formats for the block
 ** undo data, depending on whether there is a final_tx hash value specified:
 **
 ** Legacy block undo data serialization format:
 ** - std::vector<CTxUndo> vtxundo
 **
 ** Extended block undo data serialization format:
 ** - uint8_t dummy = 0xff
 ** - uint8_t flags
 ** - std::vector<CtxUndo> vtxundo
 ** - if (flags & 1)
 **   - uint256 final_tx
 **
 ** In addition, in the extended serialization format the "VARINT" format is
 ** used for encoding the number of elements for the vectors vtxundo and
 ** vtxundo.vprevout.
 **
 ** A dummy value of 0xff is used because chSize=255 in the CompactSize
 ** serialization format indicates that a 64-bit number is necessary to store
 ** the number of transactions, which is impossible without blocks being larger
 ** than 256 GiB in size.
 **/
template<typename Stream, typename Operation, typename BlockUndoType>
inline void SerializeBlockUndo(BlockUndoType& block_undo, Stream& s, Operation ser_action, int nType, int nVersion) {
    if (ser_action.ForRead()) {
        // We don't know yet if we are reading a CompactSize for the number of
        // CTxUndo structures, or the dummy value indicating an extended block
        // undo serialization format.
        uint8_t dummy = 0;
        READWRITE(dummy);
        // It is impossible to have more than 2^32 transactions in a single
        // block, so we use 0xff (which in the CompactSize format indicates a
        // 64-bit number follows) as the sentinal value indicating extended
        // serialization format.
        if (dummy == 255) {
            // The dummy value is followed by an integer flags field indicating
            // which extended parameters are present. This provides an easy
            // mechanism to extend the format in the future without a similarly
            // convoluted serialization hack. So far only one bit is used, to
            // indicate the presence of the block-final transaction hash.
            uint8_t flags = 0;
            READWRITE(flags);
            if (flags & ~0x01) {
                // Any unknown flag in the serialization causes an immediate
                // failure. This error should only be encountered by using a
                // data directory generated by a later version that defines
                // extended bits, which would require reindexing.
                throw std::ios_base::failure("Unknown flag in block undo deserialization");
            }
            // We inline the serialization of CTxUndo and CTxInUndo because the
            // extended serialization format uses the "VARINT" encoding instead
            // of "CompactSize" for vector lengths.
            uint64_t size = 0;
            READWRITE(VARINT(size));
            const_cast<std::vector<CTxUndo>*>(&block_undo.vtxundo)->resize(size);
            BOOST_FOREACH(CTxUndo& tx_undo, *const_cast<std::vector<CTxUndo>*>(&block_undo.vtxundo)) {
                READWRITE(VARINT(size));
                tx_undo.vprevout.resize(size);
                BOOST_FOREACH(CTxInUndo& txin_undo, tx_undo.vprevout) {
                    READWRITE(txin_undo);
                }
            }
            // Now we read in the optional parameters, which for the moment only
            // includes the block-final transaction hash.
            if (flags & 0x01) {
                // 32-byte hash of the prior-block's final transaction.
                READWRITE(*const_cast<uint256*>(&block_undo.final_tx));
            } else {
                // Extended serialization used, but block-final transaction hash
                // is not present. Technically this is allowed, but we will
                // never generate such a structure ourselves.
                // "Write strict, but interpret permissive."
                const_cast<uint256*>(&block_undo.final_tx)->SetNull();
            }
        }

        // Otherwise what we read was the first byte of a CompactSize integer
        // serialization for the legacy block undo structure.
        else {
            // There are some data validation checks performed when
            // deserializing a CompactSize number (see serialization.h).
            // Since we don't want to replicate that logic, we create a
            // temporary data stream with the contents of the CompactSize
            // object, and deserialize from there. It would be better to use
            // lookahead, but not all streams support that capability.
            CDataStream ds(nType, nVersion);
            ser_writedata8(ds, dummy);
            if (dummy >= 253) {
                // Either 16 bit, or the first half of 32-bit number.
                uint16_t lo = 0;
                READWRITE(lo);
                ds << lo;
            }
            if (dummy == 254) {
                // Second half of 32-bit number.
                uint16_t hi = 0;
                READWRITE(hi);
                ds << hi;
            }
            uint64_t size = ReadCompactSize(ds);
            assert(ds.empty());
            // We now read in the per-transaction undo data into the vtxundo
            // vector. Since we already read the size off the stream we inline
            // that vector serialization.
            const_cast<std::vector<CTxUndo>*>(&block_undo.vtxundo)->resize(size);
            BOOST_FOREACH(CTxUndo& tx_undo, *const_cast<std::vector<CTxUndo>*>(&block_undo.vtxundo)) {
                READWRITE(tx_undo);
            }
            // Block-final transaction hash is not used, so zero it out.
            const_cast<uint256*>(&block_undo.final_tx)->SetNull();
        }
    } else {
        // Do not use extended serialization unless we absolutely need to. This
        // preserves the ability to downgrade until the block-final transaction
        // rules activate, which causes final_tx to be set, which forces
        // extended serialization.
        uint8_t flags = !block_undo.final_tx.IsNull();
        if (flags) {
            // Write header.
            uint8_t dummy = 0xff;
            READWRITE(dummy);
            READWRITE(flags);
            // Serialize vectors with "VARINT" for size.
            uint64_t size = block_undo.vtxundo.size();
            READWRITE(VARINT(size));
            BOOST_FOREACH(const CTxUndo& tx_undo, block_undo.vtxundo) {
                size = tx_undo.vprevout.size();
                READWRITE(VARINT(size));
                BOOST_FOREACH(CTxInUndo& txin_undo, *const_cast<std::vector<CTxInUndo>*>(&tx_undo.vprevout)) {
                    READWRITE(txin_undo);
                }
            }
            // Serialize block-final transaction hash.
            if (flags & 0x01) {
                READWRITE(*const_cast<uint256*>(&block_undo.final_tx));
            }
        } else {
            // The legacy serialization format:
            READWRITE(block_undo.vtxundo);
        }
    }
}

/** Undo information for a CBlock */
class CBlockUndo
{
public:
    std::vector<CTxUndo> vtxundo; // for all but the coinbase
    uint256 final_tx; // prior block-final transaction

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        SerializeBlockUndo(*this, s, ser_action, nType, nVersion);
    }
};

#endif // FREICOIN_UNDO_H
