// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <chainparams.h>
#include <kernel/chainparams.h>
#include <coins.h>
#include <kernel/cs_main.h>
#include <serialize.h>
#include <sync.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <util/fs.h>

#include <cstdint>
#include <optional>
#include <string_view>

// UTXO set snapshot magic bytes
static constexpr std::array<uint8_t, 5> SNAPSHOT_MAGIC_BYTES = {'u', 't', 'x', 'o', 0xff};

class Chainstate;

namespace node {
//! Metadata describing a serialized version of a UTXO set from which an
//! assumeutxo Chainstate can be constructed.
//! All metadata fields come from an untrusted file, so must be validated
//! before being used. Thus, new fields should be added only if needed.
class SnapshotMetadata
{
    inline static const uint16_t VERSION{2};
    const std::set<uint16_t> m_supported_versions{VERSION};
    const MessageStartChars m_network_magic;
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

    SnapshotMetadata(
        const MessageStartChars network_magic) :
            m_network_magic(network_magic) { }
    SnapshotMetadata(
        const MessageStartChars network_magic,
        const uint256& base_blockhash,
        const BlockFinalTxEntry& final_tx,
        uint64_t coins_count) :
            m_network_magic(network_magic),
            m_base_blockhash(base_blockhash),
            m_final_tx(final_tx),
            m_coins_count(coins_count) { }

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        s << SNAPSHOT_MAGIC_BYTES;
        s << VERSION;
        s << m_network_magic;
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
        s << base_blockhash;
        // Write the extended serialization fields.
        if (flags) {
            s << flags;
            if (flags & 0x01) {
                s << m_final_tx;
            }
        }
        // Write the number of coins last.
        s << m_coins_count;
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Read the snapshot magic bytes
        std::array<uint8_t, SNAPSHOT_MAGIC_BYTES.size()> snapshot_magic;
        s >> snapshot_magic;
        if (snapshot_magic != SNAPSHOT_MAGIC_BYTES) {
            throw std::ios_base::failure("Invalid UTXO set snapshot magic bytes. Please check if this is indeed a snapshot file or if you are using an outdated snapshot format.");
        }

        // Read the version
        uint16_t version;
        s >> version;
        if (m_supported_versions.find(version) == m_supported_versions.end()) {
            throw std::ios_base::failure(strprintf("Version of snapshot %s does not match any of the supported versions.", version));
        }

        // Read the network magic (pchMessageStart)
        MessageStartChars message;
        s >> message;
        if (!std::equal(message.begin(), message.end(), m_network_magic.data())) {
            auto metadata_network{GetNetworkForMagic(message)};
            if (metadata_network) {
                std::string network_string{ChainTypeToString(metadata_network.value())};
                auto node_network{GetNetworkForMagic(m_network_magic)};
                std::string node_network_string{ChainTypeToString(node_network.value())};
                throw std::ios_base::failure(strprintf("The network of the snapshot (%s) does not match the network of this node (%s).", network_string, node_network_string));
            } else {
                throw std::ios_base::failure("This snapshot has been created for an unrecognized network. This could be a custom signet, a new testnet or possibly caused by data corruption.");
            }
        }

        s >> m_base_blockhash;
        // Check for extended serialization fields, which are indicated by
        // setting the high bit of the base blockhash.
        if (m_base_blockhash.data()[31] & 0x80) {
            m_base_blockhash.data()[31] ^= 0x80;
            unsigned char flags = 0;
            s >> flags;
            // Process each of the indicated extended serialization fields.
            if (flags & 0x01) {
                s >> m_final_tx;
                flags ^= 0x01;
            }
            // Unrecognized fields are an unrecoverable error.
            if (flags) {
                throw std::ios_base::failure("Unknown snapshot extended serialization fields");
            }
        }
        s >> m_coins_count;
    }
};

//! The file in the snapshot chainstate dir which stores the base blockhash. This is
//! needed to reconstruct snapshot chainstates on init.
//!
//! Because we only allow loading a single snapshot at a time, there will only be one
//! chainstate directory with this filename present within it.
const fs::path SNAPSHOT_BLOCKHASH_FILENAME{"base_blockhash"};

//! Write out the blockhash of the snapshot base block that was used to construct
//! this chainstate. This value is read in during subsequent initializations and
//! used to reconstruct snapshot-based chainstates.
bool WriteSnapshotBaseBlockhash(Chainstate& snapshot_chainstate)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

//! Read the blockhash of the snapshot base block that was used to construct the
//! chainstate.
std::optional<uint256> ReadSnapshotBaseBlockhash(fs::path chaindir)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

//! Suffix appended to the chainstate (leveldb) dir when created based upon
//! a snapshot.
constexpr std::string_view SNAPSHOT_CHAINSTATE_SUFFIX = "_snapshot";


//! Return a path to the snapshot-based chainstate dir, if one exists.
std::optional<fs::path> FindSnapshotChainstateDir(const fs::path& data_dir);

} // namespace node

#endif // FREICOIN_NODE_UTXO_SNAPSHOT_H
