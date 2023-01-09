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

#ifndef FREICOIN_NODE_UTXO_SNAPSHOT_H
#define FREICOIN_NODE_UTXO_SNAPSHOT_H

#include <coins.h>
#include <uint256.h>
#include <serialize.h>

namespace node {
//! Metadata describing a serialized version of a UTXO set from which an
//! assumeutxo CChainState can be constructed.
class SnapshotMetadata
{
public:
    //! The hash of the block that reflects the tip of the chain for the
    //! UTXO set contained in this snapshot.
    uint256 m_base_blockhash;

    //! The hash and number of spendable outputs of the previous
    //! block's final transaction, if Consensus::DEPLOYMENT_FINALTX is
    //! active.
    BlockFinalTxEntry m_final_tx;

    //! The number of coins in the UTXO set contained in this snapshot. Used
    //! during snapshot load to estimate progress of UTXO set reconstruction.
    uint64_t m_coins_count = 0;

    SnapshotMetadata() { }
    SnapshotMetadata(
        const uint256& base_blockhash,
        const BlockFinalTxEntry& final_tx,
        uint64_t coins_count,
        unsigned int nchaintx) :
            m_base_blockhash(base_blockhash),
            m_final_tx(final_tx),
            m_coins_count(coins_count) { }

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        // The only extended-serialization flag currently used is bit 0, which
        // if set indicates the presence of a non-null BlockFinalTxEntry field.
        unsigned char flags = !m_final_tx.IsNull();
        // The high-order bit of the base blockhash is used to signal the use of
        // extended serialization.  For all block hashes this bit will be zero,
        // so we can safely use it to convey information in the serialization
        // format.
        uint256 base_blockhash(m_base_blockhash);
        if (flags) {
            if (base_blockhash.data()[31] & 0x80) {
                // Can never happen on a real chain.  Even regtest has a minimum
                // difficulty that ensures the high-order bit is clear.
                throw std::ios_base::failure("High bit of base block hash already set");
            }
            // Set the high-order bit of the hash to indicate extended
            // serialization.
            base_blockhash.data()[31] ^= 0x80;
        }
        ::Serialize(s, base_blockhash);
        // Write the extended serialization fields.
        if (flags) {
            ::Serialize(s, flags);
            if (flags & 0x01) {
                ::Serialize(s, m_final_tx);
            }
        }
        // Write the number of coins last.
        ::Serialize(s, m_coins_count);
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        ::Unserialize(s, m_base_blockhash);
        // Check for extended serialization fields, which are indicated by
        // setting the high bit of the base blockhash.
        if (m_base_blockhash.data()[31] & 0x80) {
            m_base_blockhash.data()[31] ^= 0x80;
            unsigned char flags = 0;
            ::Unserialize(s, flags);
            // Process each of the indicated extended serialization fields.
            if (flags & 0x01) {
                ::Unserialize(s, m_final_tx);
                flags ^= 0x01;
            }
            // Unrecognized fields are an unrecoverable error.
            if (flags) {
                throw std::ios_base::failure("Unknown snapshot extended serialization fields");
            }
        }
        ::Unserialize(s, m_coins_count);
    }

    template <typename Stream>
    SnapshotMetadata(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};
} // namespace node

#endif // FREICOIN_NODE_UTXO_SNAPSHOT_H
