// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#include <script/freicoinconsensus.h>

#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <version.h>

namespace {

/** A class that deserializes a single CTransaction one time. */
class TxInputStream
{
public:
    TxInputStream(int nTypeIn, int nVersionIn, const unsigned char *txTo, size_t txToLen) :
    m_type(nTypeIn),
    m_version(nVersionIn),
    m_data(txTo),
    m_remaining(txToLen)
    {}

    void read(Span<std::byte> dst)
    {
        if (dst.size() > m_remaining) {
            throw std::ios_base::failure(std::string(__func__) + ": end of data");
        }

        if (dst.data() == nullptr) {
            throw std::ios_base::failure(std::string(__func__) + ": bad destination buffer");
        }

        if (m_data == nullptr) {
            throw std::ios_base::failure(std::string(__func__) + ": bad source buffer");
        }

        memcpy(dst.data(), m_data, dst.size());
        m_remaining -= dst.size();
        m_data += dst.size();
    }

    template<typename T>
    TxInputStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }

    int GetVersion() const { return m_version; }
private:
    const int m_type;
    const int m_version;
    const unsigned char* m_data;
    size_t m_remaining;
};

inline int set_error(freicoinconsensus_error* ret, freicoinconsensus_error serror)
{
    if (ret)
        *ret = serror;
    return 0;
}

} // namespace

/** Check that all specified flags are part of the libconsensus interface. */
static bool verify_flags(unsigned int flags)
{
    return (flags & ~(freicoinconsensus_SCRIPT_FLAGS_VERIFY_ALL)) == 0;
}

static int verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, CAmount amount, int64_t refheight,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    const UTXO *spentOutputs, unsigned int spentOutputsLen,
                                    unsigned int nIn, unsigned int flags, freicoinconsensus_error* err)
{
    if (!verify_flags(flags)) {
        return set_error(err, freicoinconsensus_ERR_INVALID_FLAGS);
    }

    if (flags & freicoinconsensus_SCRIPT_FLAGS_VERIFY_TAPROOT && spentOutputs == nullptr) {
        return set_error(err, freicoinconsensus_ERR_SPENT_OUTPUTS_REQUIRED);
    }

    try {
        TxInputStream stream(SER_NETWORK, PROTOCOL_VERSION, txTo, txToLen);
        CTransaction tx(deserialize, stream);

        std::vector<SpentOutput> spent_outputs;
        if (spentOutputs != nullptr) {
            if (spentOutputsLen != tx.vin.size()) {
                return set_error(err, freicoinconsensus_ERR_SPENT_OUTPUTS_MISMATCH);
            }
            for (size_t i = 0; i < spentOutputsLen; i++) {
                CScript spk = CScript(spentOutputs[i].scriptPubKey, spentOutputs[i].scriptPubKey + spentOutputs[i].scriptPubKeySize);
                const CAmount& value = spentOutputs[i].value;
                CTxOut tx_out = CTxOut(value, spk);
                spent_outputs.emplace_back(tx_out, spentOutputs[i].refheight);
            }
        }

        if (nIn >= tx.vin.size())
            return set_error(err, freicoinconsensus_ERR_TX_INDEX);
        if (GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION) != txToLen)
            return set_error(err, freicoinconsensus_ERR_TX_SIZE_MISMATCH);

        // Regardless of the verification result, the tx did not error.
        set_error(err, freicoinconsensus_ERR_OK);

        PrecomputedTransactionData txdata(tx);

        if (spentOutputs != nullptr && flags & freicoinconsensus_SCRIPT_FLAGS_VERIFY_TAPROOT) {
            txdata.Init(tx, std::move(spent_outputs));
        }

        return VerifyScript(tx.vin[nIn].scriptSig, CScript(scriptPubKey, scriptPubKey + scriptPubKeyLen), &tx.vin[nIn].scriptWitness, flags, TransactionSignatureChecker(&tx, nIn, amount, refheight, txdata, MissingDataBehavior::FAIL), nullptr);
    } catch (const std::exception&) {
        return set_error(err, freicoinconsensus_ERR_TX_DESERIALIZE); // Error deserializing
    }
}

int freicoinconsensus_verify_script_with_spent_outputs(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount, int64_t refheight,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    const UTXO *spentOutputs, unsigned int spentOutputsLen,
                                    unsigned int nIn, unsigned int flags, freicoinconsensus_error* err)
{
    CAmount am(amount);
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, refheight, txTo, txToLen, spentOutputs, spentOutputsLen, nIn, flags, err);
}

int freicoinconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount, int64_t refheight,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, freicoinconsensus_error* err)
{
    CAmount am(amount);
    UTXO *spentOutputs = nullptr;
    unsigned int spentOutputsLen = 0;
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, refheight, txTo, txToLen, spentOutputs, spentOutputsLen, nIn, flags, err);
}


int freicoinconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                   const unsigned char *txTo        , unsigned int txToLen,
                                   unsigned int nIn, unsigned int flags, freicoinconsensus_error* err)
{
    if (flags & freicoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS) {
        return set_error(err, freicoinconsensus_ERR_AMOUNT_REQUIRED);
    }

    CAmount am(0);
    UTXO *spentOutputs = nullptr;
    unsigned int spentOutputsLen = 0;
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, 0, txTo, txToLen, spentOutputs, spentOutputsLen, nIn, flags, err);
}

unsigned int freicoinconsensus_version()
{
    // Just use the API version for now
    return FREICOINCONSENSUS_API_VER;
}
