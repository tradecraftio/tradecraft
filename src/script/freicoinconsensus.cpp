// Copyright (c) 2009-2010 Satoshi Nakamoto
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

#include "freicoinconsensus.h"

#include "primitives/transaction.h"
#include "pubkey.h"
#include "script/interpreter.h"
#include "version.h"

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

    void read(char* pch, size_t nSize)
    {
        if (nSize > m_remaining)
            throw std::ios_base::failure(std::string(__func__) + ": end of data");

        if (pch == NULL)
            throw std::ios_base::failure(std::string(__func__) + ": bad destination buffer");

        if (m_data == NULL)
            throw std::ios_base::failure(std::string(__func__) + ": bad source buffer");

        memcpy(pch, m_data, nSize);
        m_remaining -= nSize;
        m_data += nSize;
    }

    template<typename T>
    TxInputStream& operator>>(T& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }

    int GetVersion() const { return m_version; }
    int GetType() const { return m_type; }
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

struct ECCryptoClosure
{
    ECCVerifyHandle handle;
};

ECCryptoClosure instance_of_eccryptoclosure;
}

/** Check that all specified flags are part of the libconsensus interface. */
static bool verify_flags(unsigned int flags)
{
    return (flags & ~(freicoinconsensus_SCRIPT_FLAGS_VERIFY_ALL)) == 0;
}

static int verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, CAmount amount, int64_t refheight,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, freicoinconsensus_error* err)
{
    if (!verify_flags(flags)) {
        return freicoinconsensus_ERR_INVALID_FLAGS;
    }
    try {
        TxInputStream stream(SER_NETWORK, PROTOCOL_VERSION, txTo, txToLen);
        CTransaction tx(deserialize, stream);
        if (nIn >= tx.vin.size())
            return set_error(err, freicoinconsensus_ERR_TX_INDEX);
        if (GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION) != txToLen)
            return set_error(err, freicoinconsensus_ERR_TX_SIZE_MISMATCH);

        // Regardless of the verification result, the tx did not error.
        set_error(err, freicoinconsensus_ERR_OK);

        PrecomputedTransactionData txdata(tx);
        return VerifyScript(tx.vin[nIn].scriptSig, CScript(scriptPubKey, scriptPubKey + scriptPubKeyLen), &tx.vin[nIn].scriptWitness, flags, TransactionSignatureChecker(&tx, nIn, amount, refheight, txdata), NULL);
    } catch (const std::exception&) {
        return set_error(err, freicoinconsensus_ERR_TX_DESERIALIZE); // Error deserializing
    }
}

int freicoinconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount, int64_t refheight,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, freicoinconsensus_error* err)
{
    CAmount am(amount);
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, refheight, txTo, txToLen, nIn, flags, err);
}


int freicoinconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                   const unsigned char *txTo        , unsigned int txToLen,
                                   unsigned int nIn, unsigned int flags, freicoinconsensus_error* err)
{
    if (flags & freicoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS) {
        return set_error(err, freicoinconsensus_ERR_AMOUNT_REQUIRED);
    }

    CAmount am(0);
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, 0, txTo, txToLen, nIn, flags, err);
}

unsigned int freicoinconsensus_version()
{
    // Just use the API version for now
    return FREICOINCONSENSUS_API_VER;
}
