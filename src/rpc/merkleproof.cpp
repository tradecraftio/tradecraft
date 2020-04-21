// Copyright (c) 2011-2023 The Freicoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkleproof.h>
#include <hash.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <streams.h>
#include <uint256.h>
#include <univalue.h>

#include <optional>
#include <vector>

struct MerkleElem
{
    typedef std::vector<unsigned char> data_type;
    std::optional<data_type> m_data;

    typedef uint256 hash_type;
    hash_type m_hash;

    /* The default constructor uses an empty vector as a data value.
     * An arbitrary, but reasonable choice. */
    MerkleElem(): m_data(std::vector<unsigned char>()) { Rehash(); }

    MerkleElem(const data_type& data) : m_data(data) { Rehash(); }
    MerkleElem(data_type&& data) : m_data(data) { Rehash(); }

    MerkleElem(const hash_type& hash) : m_data(std::nullopt), m_hash(hash) { }
    MerkleElem(hash_type&& hash) : m_data(std::nullopt), m_hash(hash) { }

    /* The default copy constructors and assignment operators are fine. */
    MerkleElem(const MerkleElem&) = default;
    MerkleElem(MerkleElem&&) = default;
    MerkleElem& operator=(const MerkleElem&) = default;
    MerkleElem& operator=(MerkleElem&&) = default;

    inline void SetData(const data_type& data)
        { m_data = data; Rehash(); }
    void SetData(data_type&& data)
        { m_data = data; Rehash(); }

    MerkleElem& operator=(const data_type& data)
        { SetData(data); return *this; }
    MerkleElem& operator=(data_type&& data)
        { SetData(data); return *this; }

    inline void SetHash(const hash_type& hash)
        { m_data = std::nullopt; m_hash = hash; }
    inline void SetHash(hash_type&& hash)
        { m_data = std::nullopt; m_hash = hash; }

    MerkleElem& operator=(const hash_type& hash)
        { SetHash(hash); return *this; }
    MerkleElem& operator=(hash_type&& hash)
        { SetHash(hash); return *this; }

    inline const uint256& GetHash() const
        { return m_hash; }

protected:
    void Rehash();
};

void MerkleElem::Rehash()
{
    assert(m_data != std::nullopt);
    const std::vector<unsigned char>& data = m_data.value();
    CHash256().Write(data).Finalize(m_hash);
}

static RPCHelpMan createmerkleproof()
{
    return RPCHelpMan{"createmerkleproof",
        "\nCreate a fast Merkle-tree from the provided data elements, and return the proof structure and data necessary for validation.\n",
        {
            {"data", RPCArg::Type::ARR, RPCArg::Optional::NO, "The data elements, either as hex-encoded data or hash values; see prehashed option",
                {
                    {"", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "Either hex-encoded data, or its hash value"},
                },
            },
            {"pos", RPCArg::Type::ARR, RPCArg::Optional::NO, "The position of elements which need to be verified by the proof",
                {
                    {"", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The position of an element to be verified by the proof"},
                },
            },
            {"prehashed", RPCArg::Type::BOOL, RPCArg::Default{false}, "If set, the data elements specified are hex-encoded 256-bit hash values"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "root", "The root hash of the Merkle tree"},
                {RPCResult::Type::STR_HEX, "proof", "The serialized proof structure"},
                {RPCResult::Type::ARR, "veirfy", "The data necessary to verify the proof",
                    {
                        {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "hash", "The hash used in proof verification"},
                                {RPCResult::Type::STR_HEX, "data", "The original data, if available"},
                            },
                        },
                    },
                },
            },
        },
        RPCExamples{
            HelpExampleCli("createmerkleproof", "")
          + HelpExampleRpc("createmerkleproof", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    using std::swap;

    bool prehashed = false;
    if (request.params.size() > 2) {
        prehashed = request.params[2].get_bool();
    }

    bool include_all = false;
    std::set<std::size_t> pos;
    if (request.params.size() > 1 && !request.params[1].isNull()) {
        UniValue v = request.params[1].get_array();
        for (std::size_t idx = 0; idx < v.size(); ++idx) {
            auto loc = v[idx].getInt<int>();
            if (loc < 0) {
                auto str = strprintf("Invalid tree position: %d", loc);
                throw JSONRPCError(RPC_INVALID_PARAMETER, str);
            }
            if (pos.count(loc)) {
                auto str = strprintf("Tree position specified twice: %d", loc);
                throw JSONRPCError(RPC_INVALID_PARAMETER, str);
            }
            pos.insert(loc);
        }
    } else {
        include_all = true;
    }

    std::vector<MerkleElem> data;
    std::vector<MerkleTree> tree;
    if (request.params.size() > 0 && !request.params[0].isNull()) {
        UniValue v = request.params[0].get_array();
        std::size_t idx = 0;
        for (; idx < v.size(); ++idx) {
            if (prehashed) {
                MerkleElem::hash_type h256;
                h256.SetHex(v[idx].get_str());
                data.emplace_back(std::move(h256));
            } else {
                MerkleElem::data_type vch = ParseHex(v[idx].get_str());
                data.emplace_back(std::move(vch));
            }
            tree.emplace_back(data.back().GetHash(), include_all || pos.count(idx));
        }
        // Now that we know the number of elements in the tree, we can
        // check whether any of the tree position locators given in
        // request.params[1] are out of range.
        std::set<std::size_t> invalid_pos;
        for (auto loc : pos) {
            if (loc >= idx) {
                invalid_pos.insert(loc);
            }
        }
        if (!invalid_pos.empty()) {
            std::string str("These tree positions are out of range: ");
            for (auto loc : invalid_pos) {
                str += strprintf("%d, ", loc);
            }
            str.erase(str.size()-2, 2); // ", "
            throw JSONRPCError(RPC_INVALID_PARAMETER, str);
        }
    }

    // If called with no parameters, we return the "empty" proof.
    if (tree.empty()) {
        tree.resize(1);
    }

    while (tree.size() > 1) {
        std::vector<MerkleTree> next;
        for (std::size_t idx = 0; idx < tree.size(); idx += 2) {
            std::size_t idx2 = idx + 1;
            if (idx2 < tree.size()) {
                next.emplace_back(tree[idx], tree[idx2]);
            } else {
                next.emplace_back(tree[idx]);
            }
        }
        swap(tree, next);
    }

    assert(tree.size() == 1);
    CDataStream ssProof(SER_NETWORK, PROTOCOL_VERSION);
    ssProof << tree[0].m_proof;

    bool invalid = true;
    std::vector<MerkleBranch> proofs;
    uint256 root = tree[0].GetHash(&invalid, &proofs);
    if (invalid) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Internal error: invalid proof generated.");
    }
    if (proofs.size() != tree[0].m_verify.size()) {
        auto str = strprintf("Internal error: wrong number of proofs returned (expected %d, got %d)", tree[0].m_verify.size(), proofs.size());
        throw JSONRPCError(RPC_INVALID_PARAMETER, str);
    }

    UniValue verify(UniValue::VARR);
    for (std::size_t i = 0; i < tree[0].m_verify.size(); ++i) {
        const uint256& hash = tree[0].m_verify[i];
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("hash", hash.GetHex());
        for (auto elem : data) {
            if (hash == elem.GetHash() && elem.m_data != std::nullopt) {
                obj.pushKV("data", HexStr(elem.m_data.value()));
                break;
            }
        }
        obj.pushKV("proof", HexStr(proofs[i].getvch()));
        verify.push_back(obj);
    }

    UniValue res(UniValue::VOBJ);
    res.pushKV("root", root.GetHex());
    res.pushKV("tree", HexStr(ssProof));
    res.pushKV("verify", verify);

    return res;
},
    };
}

void RegisterMerkleProofRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"util", &createmerkleproof},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
