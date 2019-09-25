// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
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

#ifndef FREICOIN_PRIMITIVES_TRANSACTION_H
#define FREICOIN_PRIMITIVES_TRANSACTION_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "streams.h"
#include "uint256.h"

static const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;

static const int WITNESS_SCALE_FACTOR = 4;

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, uint32_t nIn) { hash = hashIn; n = nIn; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1 << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(prevout);
        READWRITE(*(CScriptBase*)(&scriptSig));
        READWRITE(nSequence);
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
/**
 * The vast majority of the accesses to CTxOut::nValue are from unit
 * tests. Rather than update all of these to use the setters/getters
 * and jump through hoops to set exact values, we make the nValue
 * field public when compiling unit tests.
 */
#ifdef FREICOIN_TEST
public:
#else
protected:
#endif
    CAmount nValue;

public:
    CScript scriptPubKey;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nValue);
        READWRITE(*(CScriptBase*)(&scriptPubKey));
    }

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
    }

    bool IsNull() const
    {
        return (nValue == -1);
    }

    uint256 GetHash() const;

    /**
     * The value of an input in freicoin is adjusted based on the
     * difference between the reference height of the transaction
     * which spends it and the transaction which created it. In
     * updating any code from bitcoin which accesses a CTxOut's
     * nValue, the value might need to be adjusted to either the
     * spending reference height or the present. In order to make sure
     * that no instance gets through accidentally unconsidered, we
     * force the access of this value through getters and setters.
     *
     * Generally speaking, access to CTxOut::nValue falls into one of
     * three categories:
     *
     * 1. Historical records or reports of a transaction, which are
     *    recorded at the reference height of the transaction. The
     *    historical fact that a transaction debits 10frc from a
     *    wallet does not change over time.
     *
     * 2. Inputs to other transactions, in which case the value of the
     *    input is decayed by demurrage according to the difference
     *    between the two transactions reference heights.
     *
     * 3. Lists of unspent outputs or wallet balances, which report
     *    each unspent output as decayed to present value (the height
     *    of the next block), for coin selection and available balance
     *    purposes.
     *
     * The only odd thing worth noting is the interaction between
     * account or address views. With Freicoin the decision was made
     * that these views provide an aggregate view of historical
     * records. So if you wallet has received 100frc that is an
     * average of 1 year old, `getbalance` would return ~95frc, but
     * `listreceivedbyaddress` would show a per-address view of
     * historical amounts which if summed would yield 100frc.
     */
    CTxOut& SetReferenceValue(const CAmount& value_in)
    {
        nValue = value_in;
        return *this;
    }

    /* There are a few points in transaction creation logic where a
     * CTxOut::nValue is adjusted by some amount, e.g. to subtract
     * fee. This method exists to make those scenarios a little bit
     * more concise. */
    CTxOut& AdjustReferenceValue(const CAmount& delta)
    {
        nValue += delta;
        return *this;
    }

    CAmount GetReferenceValue() const
    {
        return nValue;
    }

    CAmount GetTimeAdjustedValue(int relative_depth) const
    {
        return ::GetTimeAdjustedValue(nValue, relative_depth);
    }

    CAmount GetDustThreshold(const CFeeRate &minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee,
        // which has units kria-per-kilobyte.
        // If you'd pay more than 1/3 in fees
        // to spend something, then we consider it dust.
        // A typical spendable non-segwit txout is 34 bytes big, and will
        // need a CTxIn of at least 148 bytes to spend:
        // so dust is a spendable txout less than
        // 546*minRelayTxFee/1000 (in kria).
        // A typical spendable segwit txout is 31 bytes big, and will
        // need a CTxIn of at least 67 bytes to spend:
        // so dust is a spendable txout less than
        // 294*minRelayTxFee/1000 (in kria).
        if (scriptPubKey.IsUnspendable())
            return 0;

        size_t nSize = GetSerializeSize(SER_DISK, 0);
        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;

        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            // sum the sizes of the parts of a transaction input
            // with 75% segwit discount applied to the script size.
            nSize += (32 + 4 + 1 + (107 / WITNESS_SCALE_FACTOR) + 4);
        } else {
            nSize += (32 + 4 + 1 + 107 + 4); // the 148 mentioned above
        }

        return 3 * minRelayTxFee.GetFee(nSize);
    }

    bool IsDust(const CFeeRate &minRelayTxFee) const
    {
        return (nValue < GetDustThreshold(minRelayTxFee));
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

class CTxInWitness
{
public:
    CScriptWitness scriptWitness;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(scriptWitness.stack);
    }

    bool IsNull() const { return scriptWitness.IsNull(); }

    CTxInWitness() { }
};

class CTxWitness
{
public:
    /** In case vtxinwit is missing, all entries are treated as if they were empty CTxInWitnesses */
    std::vector<CTxInWitness> vtxinwit;

    ADD_SERIALIZE_METHODS;

    bool IsEmpty() const { return vtxinwit.empty(); }

    bool IsNull() const
    {
        for (size_t n = 0; n < vtxinwit.size(); n++) {
            if (!vtxinwit[n].IsNull()) {
                return false;
            }
        }
        return true;
    }

    void SetNull()
    {
        vtxinwit.clear();
    }

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        for (size_t n = 0; n < vtxinwit.size(); n++) {
            READWRITE(vtxinwit[n]);
        }
        if (IsNull()) {
            /* It's illegal to encode a witness when all vtxinwit entries are empty. */
            throw std::ios_base::failure("Superfluous witness record");
        }
    }
};

struct CMutableTransaction;

/**
 * Basic transaction serialization format:
 * - int32_t nVersion
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - uint32_t nLockTime
 *
 * Extended transaction serialization format:
 * - int32_t nVersion
 * - unsigned char dummy = 0xff
 * - unsigned char flags (!= 0)
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - if (flags & 1):
 *   - CTxWitness wit;
 * - uint32_t nLockTime
 */
template<typename Stream, typename Operation, typename TxType>
inline void SerializeTransaction(TxType& tx, Stream& s, Operation ser_action, int nType, int nVersion) {
    READWRITE(*const_cast<int32_t*>(&tx.nVersion));
    unsigned char flags = 0;
    if (ser_action.ForRead()) {
        const_cast<std::vector<CTxIn>*>(&tx.vin)->clear();
        const_cast<std::vector<CTxOut>*>(&tx.vout)->clear();
        const_cast<CTxWitness*>(&tx.wit)->SetNull();
        // We don't know yet if we are reading a CompactSize for the number of
        // CTxIn structures, or the dummy value indicating an extended
        // transaction serialization format.
        unsigned char dummy = 0;
        unsigned char flags = 0;
        READWRITE(dummy);
        // It is impossible to have more than 2^32 inputs in a single
        // transaction, so we use 0xff (which in the CompactSize format
        // indicates a 64-bit number follows) as the sentinal value indicating
        // extended serialization format.
        if (dummy == 255) {
            // The dummy value is followed by an integer flags field indicating
            // which extended parameters are present. This provides an easy
            // mechanism to extend the format in the future without a similarly
            // convoluted serialization hack. So far only one bit is used, to
            // indicate the presence of the input witness vector.
            READWRITE(flags);
            // The input vector is read by the other branch (since the "dummy"
            // value was the first part of its size), so to synchronize state we
            // also read in the vector here.
            READWRITE(*const_cast<std::vector<CTxIn>*>(&tx.vin));
        }
        // Otherwise what we read was the first byte of a CompactSize integer
        // serialization of the length of the input vector in the legacy
        // transaction serialization structure.
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
            // We now read in the CTxIn data into the vin vector. Since we
            // already read the size off the stream we inline that vector
            // serialization.
            const_cast<std::vector<CTxIn>*>(&tx.vin)->resize(size);
            for (CTxIn& txin : *const_cast<std::vector<CTxIn>*>(&tx.vin)) {
                READWRITE(txin);
            }
        }
        // Reading vout requires no special handling.
        READWRITE(*const_cast<std::vector<CTxOut>*>(&tx.vout));
        if ((flags & 1) && !(nVersion & SERIALIZE_TRANSACTION_NO_WITNESS)) {
            /* The witness flag is present, and we support witnesses. */
            flags ^= 1;
            const_cast<CTxWitness*>(&tx.wit)->vtxinwit.resize(tx.vin.size());
            READWRITE(tx.wit);
        }
        if (flags) {
            /* Unknown flag in the serialization */
            throw std::ios_base::failure("Unknown transaction optional data");
        }
    } else {
        // Consistency check
        assert(tx.wit.vtxinwit.size() <= tx.vin.size());
        if (!(nVersion & SERIALIZE_TRANSACTION_NO_WITNESS)) {
            /* Check whether witnesses need to be serialized. */
            if (!tx.wit.IsNull()) {
                flags |= 1;
            }
        }
        if (flags) {
            /* Use extended format in case witnesses are to be serialized. */
            unsigned char dummy = 255;
            READWRITE(dummy);
            READWRITE(flags);
        }
        READWRITE(*const_cast<std::vector<CTxIn>*>(&tx.vin));
        READWRITE(*const_cast<std::vector<CTxOut>*>(&tx.vout));
        if (flags & 1) {
            const_cast<CTxWitness*>(&tx.wit)->vtxinwit.resize(tx.vin.size());
            READWRITE(tx.wit);
        }
    }
    READWRITE(*const_cast<uint32_t*>(&tx.nLockTime));
    if (tx.nVersion != 1 || tx.vin.size() != 1 || !tx.vin[0].prevout.IsNull()) {
        READWRITE(*const_cast<int32_t*>(&tx.lock_height));
    } else {
        if (ser_action.ForRead()) {
            *const_cast<int32_t*>(&tx.lock_height) = 0;
        }
    }
}

/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
private:
    /** Memory only. */
    const uint256 hash;

public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION=2;

    // Changing the default transaction version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    static const int32_t MAX_STANDARD_VERSION=2;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const int32_t nVersion;
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    CTxWitness wit; // Not const: can change without invalidating the txid cache
    const uint32_t nLockTime;
    // Note: It would be semantically better to use a uint32_t here.
    //       However there are many places where the lock_neight is
    //       compared against the chain height, which is a regular
    //       signed int. Converting between the two in a way that
    //       avoids implementation defined semantics is actually
    //       quicte tricky and verbose. At some point we should change
    //       that logic to be unsigned as well, and then we can change
    //       this to a uint32_t. That would technically be a
    //       hard-fork, but one that triggers 40,000 years in the
    //       future, which is perfectly fine. All chain progress would
    //       stop at that point anyway for similar reasons related to
    //       height overflow.
    const int32_t lock_height;

    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    CTransaction(const CMutableTransaction &tx);

    CTransaction& operator=(const CTransaction& tx);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        SerializeTransaction(*this, s, ser_action, nType, nVersion);
        if (ser_action.ForRead()) {
            UpdateHash();
        }
    }

    bool IsNull() const {
        return vin.empty() && vout.empty();
    }

    const uint256& GetHash() const {
        return hash;
    }

    // Compute a hash that includes both transaction and witness data
    uint256 GetWitnessHash() const;

    // Return sum of txouts.
    CAmount GetValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    CAmount GetPresentValueOfOutput(int n, uint32_t height) const
    {
        assert(n < (int)vout.size());
        return vout[n].GetTimeAdjustedValue((int)height - lock_height);
    }

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    // Compute modified tx size for priority calculation (optionally given tx size)
    unsigned int CalculateModifiedSize(unsigned int nTxSize=0) const;

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;

    void UpdateHash() const;
};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    int32_t nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    CTxWitness wit;
    uint32_t nLockTime;
    int32_t lock_height;

    CMutableTransaction();
    CMutableTransaction(const CTransaction& tx);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        SerializeTransaction(*this, s, ser_action, nType, nVersion);
    }

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;
};

/** Compute the weight of a transaction, as defined by BIP 141 */
int64_t GetTransactionWeight(const CTransaction &tx);

#endif // FREICOIN_PRIMITIVES_TRANSACTION_H
