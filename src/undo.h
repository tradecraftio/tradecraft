// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UNDO_H
#define BITCOIN_UNDO_H

#include "compressor.h" 
#include "consensus/consensus.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "streams.h"

/** Undo information for a CTxIn
 *
 *  Contains the prevout's CTxOut being spent, and its metadata as well
 *  (coinbase or not, height). The serialization contains a dummy value of
 *  zero. This is be compatible with older versions which expect to see
 *  the transaction version there.
 */
class TxInUndoSerializer
{
    const Coin* txout;

public:
    template<typename Stream>
    void Serialize(Stream &s) const {
        ::Serialize(s, VARINT(txout->nHeight * 2 + (txout->fCoinBase ? 1 : 0)));
        if (txout->nHeight > 0) {
            // Required to maintain compatibility with older undo format.
            ::Serialize(s, (unsigned char)0);
        }
        ::Serialize(s, CTxOutCompressor(REF(txout->out)));
    }

    TxInUndoSerializer(const Coin* coin) : txout(coin) {}
};

class TxInUndoDeserializer
{
    Coin* txout;

public:
    template<typename Stream>
    void Unserialize(Stream &s) {
        unsigned int nCode = 0;
        ::Unserialize(s, VARINT(nCode));
        txout->nHeight = nCode / 2;
        txout->fCoinBase = nCode & 1;
        if (txout->nHeight > 0) {
            // Old versions stored the version number for the last spend of
            // a transaction's outputs. Non-final spends were indicated with
            // height = 0.
            int nVersionDummy;
            ::Unserialize(s, VARINT(nVersionDummy));
        }
        ::Unserialize(s, REF(CTxOutCompressor(REF(txout->out))));
    }

    TxInUndoDeserializer(Coin* coin) : txout(coin) {}
};

static const size_t MIN_TRANSACTION_INPUT_WEIGHT = WITNESS_SCALE_FACTOR * ::GetSerializeSize(CTxIn(), SER_NETWORK, PROTOCOL_VERSION);
static const size_t MAX_INPUTS_PER_BLOCK = MAX_BLOCK_WEIGHT / MIN_TRANSACTION_INPUT_WEIGHT;
static const size_t MAX_TX_PER_BLOCK = (MAX_BLOCK_WEIGHT / 4) / 50;

/** Undo information for a CTransaction */
class TxUndoSerializer
{
    const std::vector<Coin>& vprevout;

public:
    template<typename Stream>
    void Serialize(Stream &s) const {
        // TODO: avoid reimplementing vector serializer
        uint64_t count = vprevout.size();
        ::Serialize(s, COMPACTSIZE(REF(count)));
        for (const auto& prevout : vprevout) {
            ::Serialize(s, REF(TxInUndoSerializer(&prevout)));
        }
    }

    TxUndoSerializer(const std::vector<Coin>& vprevoutIn) : vprevout(vprevoutIn) {}
};

class TxUndoDeserializer
{
    std::vector<Coin>& vprevout;

public:
    template<typename Stream>
    void Unserialize(Stream &s) {
        // TODO: avoid reimplementing vector deserializer
        uint64_t count = 0;
        ::Unserialize(s, COMPACTSIZE(count));
        if (count > MAX_INPUTS_PER_BLOCK) {
            throw std::ios_base::failure("Too many input undo records");
        }
        vprevout.resize(count);
        for (auto& prevout : vprevout) {
            ::Unserialize(s, REF(TxInUndoDeserializer(&prevout)));
        }
    }

    TxUndoDeserializer(std::vector<Coin>& vprevoutIn) : vprevout(vprevoutIn) {}
};

class CTxUndo
{
public:
    // undo information for all txins
    std::vector<Coin> vprevout;

    template <typename Stream>
    void Serialize(Stream& s) const {
        ::Serialize(s, REF(TxUndoSerializer(vprevout)));
    }

    template <typename Stream>
    void Unserialize(Stream& s) {
        ::Unserialize(s, REF(TxUndoDeserializer(vprevout)));
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
class BlockUndoSerializer
{
    const std::vector<CTxUndo>& vtxundo; // for all but the coinbase
    const BlockFinalTxEntry& final_tx; // prior block-final transaction

public:
    template<typename Stream>
    void Serialize(Stream &s) const {
        // Do not use extended serialization unless we absolutely need to. This
        // preserves the ability to downgrade until the block-final transaction
        // rules activate, which causes final_tx to be set, which forces
        // extended serialization.
        uint8_t flags = !final_tx.IsNull();
        if (flags) {
            // Write header.
            uint8_t dummy = 0xff;
            ::Serialize(s, dummy);
            ::Serialize(s, flags);
            // Serialize vectors with "VARINT" for size.
            uint64_t count = vtxundo.size();
            ::Serialize(s, VARINT(count));
            for (const auto& tx_undo : vtxundo) {
                count = tx_undo.vprevout.size();
                ::Serialize(s, VARINT(count));
                for (const auto& txin_undo : tx_undo.vprevout) {
                    ::Serialize(s, REF(TxInUndoSerializer(&txin_undo)));
                }
            }
            // Serialize block-final transaction hash.
            if (flags & 0x01) {
                ::Serialize(s, final_tx);
            }
        } else {
            // The legacy serialization format:
            // TODO: avoid reimplementing vector serializer
            uint64_t count = vtxundo.size();
            ::Serialize(s, COMPACTSIZE(REF(count)));
            for (const auto& txundo : vtxundo) {
                ::Serialize(s, REF(TxUndoSerializer(txundo.vprevout)));
            }
        }
    }

    BlockUndoSerializer(const std::vector<CTxUndo>& vtxundoIn, const BlockFinalTxEntry& finalTxEntry) : vtxundo(vtxundoIn), final_tx(finalTxEntry) {}
};

class BlockUndoDeserializer
{
    std::vector<CTxUndo>& vtxundo; // for all but the coinbase
    BlockFinalTxEntry& final_tx; // prior block-final transaction

public:
    template<typename Stream>
    void Unserialize(Stream &s) {
        // We don't know yet if we are reading a CompactSize for the number of
        // CTxUndo structures, or the dummy value indicating an extended block
        // undo serialization format.
        uint8_t dummy = 0;
        ::Unserialize(s, dummy);
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
            ::Unserialize(s, flags);
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
            uint64_t count = 0;
            ::Unserialize(s, VARINT(count));
            if (count > MAX_TX_PER_BLOCK) {
                throw std::ios_base::failure("Too many tx undo records");
            }
            vtxundo.resize(count);
            for (auto& tx_undo : vtxundo) {
                ::Unserialize(s, VARINT(count));
                if (count > MAX_INPUTS_PER_BLOCK) {
                    throw std::ios_base::failure("Too many input undo records");
                }
                tx_undo.vprevout.resize(count);
                for (auto& txin_undo : tx_undo.vprevout) {
                    ::Unserialize(s, REF(TxInUndoDeserializer(&txin_undo)));
                }
            }
            // Now we read in the optional parameters, which for the moment only
            // includes the block-final transaction hash.
            if (flags & 0x01) {
                // 32-byte hash of the prior-block's final transaction.
                ::Unserialize(s, final_tx);
            } else {
                // Extended serialization used, but block-final transaction hash
                // is not present. Technically this is allowed, but we will
                // never generate such a structure ourselves.
                // "Write strict, but interpret permissive."
                final_tx.SetNull();
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
            CDataStream ds(SER_GETHASH, 0);
            ser_writedata8(ds, dummy);
            if (dummy >= 253) {
                // Either 16 bit, or the first half of 32-bit number.
                uint16_t lo = 0;
                ::Unserialize(s, lo);
                ds << lo;
            }
            if (dummy == 254) {
                // Second half of 32-bit number.
                uint16_t hi = 0;
                ::Unserialize(s, hi);
                ds << hi;
            }
            uint64_t count = ReadCompactSize(ds);
            assert(ds.empty());
            // We now read in the per-transaction undo data into the vtxundo
            // vector. Since we already read the size off the stream we inline
            // that vector serialization.
            if (count > MAX_TX_PER_BLOCK) {
                throw std::ios_base::failure("Too many tx undo records");
            }
            vtxundo.resize(count);
            for (auto& txundo : vtxundo) {
                ::Unserialize(s, REF(TxUndoDeserializer(txundo.vprevout)));
            }
            // Block-final transaction hash is not used, so zero it out.
            final_tx.SetNull();
        }
    }

    BlockUndoDeserializer(std::vector<CTxUndo>& vtxundoIn, BlockFinalTxEntry& finalTxEntry) : vtxundo(vtxundoIn), final_tx(finalTxEntry) {}
};

/** Undo information for a CBlock */
class CBlockUndo
{
public:
    std::vector<CTxUndo> vtxundo; // for all but the coinbase
    BlockFinalTxEntry final_tx; // prior block-final transaction

    template <typename Stream>
    void Serialize(Stream& s) const {
        ::Serialize(s, REF(BlockUndoSerializer(vtxundo, final_tx)));
    }

    template <typename Stream>
    void Unserialize(Stream& s) {
        ::Unserialize(s, REF(BlockUndoDeserializer(vtxundo, final_tx)));
    }
};

#endif // BITCOIN_UNDO_H
