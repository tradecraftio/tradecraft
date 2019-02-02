// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2011-2018 The Freicoin developers
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

#include "core.h"

#include "util.h"

int64_t TimeAdjustValueForward(int64_t initial_value, uint32_t distance)
{
    /* We accept a signed initial_value as input, but perform
     * demurrage calculations on that value's absolute magnitude. */
    const int sign = (initial_value > 0) - (initial_value < 0);
    const uint64_t value = std::abs(initial_value);

    /* The demurrage rate for an offset of 0 blocks, which is 1.0
     * exactly, has no representation in 0.64 fixed point. */
    if (distance == 0)
        return initial_value;
    /* A distance of 2^26 blocks and beyond are sufficient to decay
     * even MAX_MONEY to zero. */
    if (distance >= ((uint32_t)1<<26))
        return 0;

    /* This array of pre-generated constants is an exponentiation
     * ladder of properly calculated 64-bit fixed point demurrage
     * rates for power-of-2 block intervals. Calculating the actual
     * demurrage rate for the passed in distance is a matter of
     * performing fixed point multiplication of the factors
     * corresponding to the powers of 2 (set bits) which make up
     * distance.
     *
     * Our lookup table does not go beyond 26 entries because a
     * distance of 1<<26 (the would-be 27th entry) would cause even
     * MAX_MONEY (2^53 - 1) to decay to zero. If we are given a
     * distance value greater than or equal to (1<<26), we simply
     * return 0LL. */
    static const uint32_t k32[2*26] = {
        0xfffff000, 0x00000000, /* 2^0 = 1 */
        0xffffe000, 0x01000000, /* 2^1 = 2 */
        0xffffc000, 0x05ffffc0, /* 2^2 = 4 */
        0xffff8000, 0x1bfffc80, /* 2^3 = 8 */
        0xffff0000, 0x77ffdd00, /* ... */
        0xfffe0001, 0xeffeca00,
        0xfffc0007, 0xdff5d409,
        0xfff8001f, 0xbfaca8a2,
        0xfff0007f, 0x7d5d5a6a,
        0xffe001fe, 0xeacb48a8,
        0xffc007fd, 0x55dfda2a,
        0xff801ff6, 0xad5499cd,
        0xff007fcd, 0x67f98aad,
        0xfe01fe9b, 0x74f0943e,
        0xfc07f540, 0x767d2a82,
        0xf81fab16, 0x3dc15990,
        0xf07d5f65, 0xf9604ac9,
        0xe1eb5045, 0x80b6ebf7,
        0xc75f7b66, 0xa5075def,
        0x9b459576, 0x663bbb3e,
        0x5e2d55e7, 0x48e27ab4,
        0x22a5531d, 0x29a95916,
        0x04b054d7, 0xfda49c4d,
        0x0015fc1b, 0x85085be9,
        0x000001e3, 0x54ca043c,
        0x00000000, 0x00039089 };

    /* Overflow sensitive fixed point multiply-and-accumulate methods
     * which are used both for the exponentiation to calcuate the
     * demurrage rate, and the final multiply at the end. */
    uint64_t sum=0, overflow=0;
    #define shift32() do { \
        sum = (overflow << 32) + (sum >> 32); \
        overflow = 0; \
    } while (0)
    #define term(_val) do { \
        const uint64_t val = (_val); \
        overflow += (sum + val) < sum; \
        sum += val; \
    } while (0)

    /* We calculate the first 64 fractional bits of the aggregate
     * demurrage rate over distance blocks by raising the per-block
     * rate of (1 - 2^-20) to the distance'th power. To perform this
     * computation efficiently we perform N multiplications out of a
     * pre-computed exponentiation ladder, where N is the number of
     * set bits in the binary representation of distance. */

    /* At the end of this calculation w will contain the first 64
     * fractional bits of the demurrage rate as a pair of 32-bit
     * unsigned words, the most significant word first. Its initial
     * value is multiplicative identity, 1.0, for which the fractional
     * bits are zero. */
    uint32_t w[2] = { 0 };

    /* The first multiplication has the accumulator set to 1.0, which
     * is the only time it has a value >= 1. Since we don't store the
     * non-fractional bits, we need to special-case the first
     * multiplication. */
    bool first = true;

    for (int bit = 0; distance; distance >>= 1, ++bit) {
        if (distance & 1) {
            /* The first time through the accumulator has the value
             * 1.0.  Multiplication by 1.0 is easy--just copy the
             * value from the table. */
            if (first) {
                first = false;
                w[0] = k32[2*bit];
                w[1] = k32[2*bit+1];
                continue;
            }

            /* Zero-extend the accumulator state and ladder entry as
             * we will be doing our multiplications in 64-bit to
             * capture to full range. */
            const uint64_t w0 = w[0];
            const uint64_t w1 = w[1];
            const uint64_t k0 = k32[2*bit];
            const uint64_t k1 = k32[2*bit+1];

            /* Carry out the multiplication, term-by-term. Terms whose
             * contribution to the final result are entirely wiped
             * away by truncation are not included. */

            overflow = 0;
            sum= k1 * w0;
            term(k0 * w1);
            shift32();
            term(k0 * w0);
            w[1] = static_cast<uint32_t>(sum);
            shift32();

            w[0] = static_cast<uint32_t>(sum);

            /* Debugging check: under no circumstances should it ever
             * be the case that the high-order bits of sum (including
             * detected overflow) are non-zero. That would indicate
             * that the multiplication resulted in a value of 1.0 or
             * greater, which shouldn't be possile. */
            // shift32();
            // assert(sum == 0);
        }
    }

    /* We now perform an approximately similar multiplication of the
     * final calculated demurrage factor by the passed in value. */
    const uint64_t v0 = value >> 32;
    const uint64_t v1 = static_cast<uint32_t>(value);

    overflow = 0;
    sum = (w[1] * v1) >> 32;
    term(w[1] * v0);
    term(w[0] * v1);
    shift32();
    term(w[0] * v0);

    #undef term
    #undef shift32

    /* Debugging check: having the overflow bit set at this point
     * would indicate that the demurrage calculation has resulted in
     * an amount that is greater than std::numeric_limits<int64_t>::
     * max(), which should never be possible as the demurrage factor
     * should always be a fractional number less than one. */
    // assert(overflow == 0);

    /* Debugging check: the semantics of time-adjustment are that it
     * never returns a value with magnitude outside the range of [0,
     * MAX_MONEY]. But the only way this could happen is if we are
     * passed a value greater than MAX_MONEY, which is outside the
     * domain of this function. So we don't need to check for this in
     * production */
    // assert(result <= MAX_MONEY);

    return sign * static_cast<int64_t>(sum);
}

int64_t TimeAdjustValueReverse(int64_t initial_value, uint32_t distance)
{
    /* We accept a signed initial_value as input, but perform
     * demurrage calculations on that value's absolute magnitude. */
    const int sign = (initial_value > 0) - (initial_value < 0);
    const uint64_t value = std::abs(initial_value);

    /* Later on we might return +/- MAX_MONEY in cases of overflow.
     * The one instance in which this is incorrect behavior is when
     * the input value is zero, so we must handle that as a special
     * case first. */
    if (value == 0)
        return 0;

    /* A distance of 2^26 blocks and beyond are sufficient to decay
     * even MAX_MONEY to zero going forward, which in reverse implies
     * d a single kria would exceed MAX_MONEY. */
    const int64_t kOverflow = sign * MAX_MONEY;
    if (distance >= ((uint32_t)1<<26))
        return kOverflow;

    /* These arrays of pre-generated constants are an exponentiation
     * ladder of properly calculated 64.64-bit fixed point inverse
     * demurrage factors for power-of-2 block intervals. Calculating
     * the the aggregate inverse demurrage factor for the given
     * distance is a matter of performing fixed point multiplication
     * of the factors corresponding to the powers of 2 (set bits) in
     * the binary representation of distance.
     *
     * Our lookup table does not go beyond 26 entries because a
     * distance of 2^26 blocks (the would-be 27th entry) would cause
     * any input value (except zero) to overflow. */
    static const uint32_t k32[4*26] = {
        0x00000000, 0x00000001, 0x00001000, 0x01000010, /* -2^0 = -1 */
        0x00000000, 0x00000001, 0x00002000, 0x03000040, /* -2^1 = -2 */
        0x00000000, 0x00000001, 0x00004000, 0x0a000140, /* -2^2 = -4 */
        0x00000000, 0x00000001, 0x00008000, 0x24000780, /* -2^3 = -8 */
        0x00000000, 0x00000001, 0x00010000, 0x88003300, /* ... */
        0x00000000, 0x00000001, 0x00020002, 0x10017600,
        0x00000000, 0x00000001, 0x00040008, 0x200b2c0b,
        0x00000000, 0x00000001, 0x00080020, 0x405758b2,
        0x00000000, 0x00000001, 0x00100080, 0x82b2baeb,
        0x00000000, 0x00000001, 0x00200201, 0x15760cb0,
        0x00000000, 0x00000001, 0x00400802, 0xab357b3b,
        0x00000000, 0x00000001, 0x00802009, 0x5800bbef,
        0x00000000, 0x00000001, 0x01008032, 0xbd5bcef3,
        0x00000000, 0x00000001, 0x02020166, 0x20651cee,
        0x00000000, 0x00000001, 0x04080ad5, 0xdee644e3,
        0x00000000, 0x00000001, 0x08205643, 0x1a97126a,
        0x00000000, 0x00000001, 0x1082b600, 0x14af6333,
        0x00000000, 0x00000001, 0x2216057d, 0x856dd258,
        0x00000000, 0x00000001, 0x48b5e655, 0x53fde431,
        0x00000000, 0x00000001, 0xa6129f7a, 0x2b20cd20,
        0x00000000, 0x00000002, 0xb7e16721, 0x96b730c5,
        0x00000000, 0x00000007, 0x6399a46e, 0xd2eda481,
        0x00000000, 0x00000036, 0x99272f73, 0x36391a9f,
        0x00000000, 0x00000ba4, 0xf827e152, 0x14cd8421,
        0x00000000, 0x008797a2, 0x510309b9, 0xc64e0d7e,
        0x000047d1, 0x470253b0, 0x78e38992, 0x14983b4b };

    /* Overflow sensitive fixed point multiply-and-accumulate methods
     * which are used both for the exponentiation to calcuate the
     * aggregate demurrage rate, and the final multiply at the end. */
    uint64_t sum=0, overflow=0;
    #define shift32() do { \
        sum = (overflow << 32) + (sum >> 32); \
        overflow = 0; \
    } while (0)
    #define term(_val) do { \
        const uint64_t val = (_val); \
        overflow += (sum + val) < sum; \
        sum += val; \
    } while (0)

    /* We calculate the aggregate inverse demurrage factor for
     * distance by raising the per-block rate of 1/(1 - 2^-20) to the
     * distance'th power. To perform this computation efficiently we
     * perform N multiplications of a pre-computed exponentiation
     * ladder, where N is the number of set bits in the binary
     * representation of distance. */

    /* At the end of this calculation the 64.64-bit fixed-point number
     * w will contain a representation of the demurrage or inverse
     * demurrage rate as a quad of 32-bit unsigned words, the most
     * significant word first. Its initial value is multiplicative
     * identity, 1.0, for which the fractional bits are zero. */
    uint32_t w[4] = { 0, 1, 0, 0 };

    /* The first multiplication has the accumulator set to 1.0. So as
     * an optimization we don't need to perform a term-by-term
     * multiplication, but can instead just copy the factor into the
     * accumulator. */
    bool first = true;

    for (int bit = 0; distance; distance >>= 1, ++bit) {
        if (distance & 1) {
            /* w contains the multiplicative identity, so the first
             * time a bit is set we simply copy the relevant factor
             * into the accumulator. */
            if (first) {
                first = false;
                w[0] = k32[4*bit];
                w[1] = k32[4*bit+1];
                w[2] = k32[4*bit+2];
                w[3] = k32[4*bit+3];
                continue;
            }

            /* Zero-extend the accumulator state and ladder entry as
             * we will be doing our multiplications in 64-bit to
             * capture full range and detect overflow. */
            const uint64_t w0 = w[0];
            const uint64_t w1 = w[1];
            const uint64_t w2 = w[2];
            const uint64_t w3 = w[3];
            const uint64_t k0 = k32[4*bit];
            const uint64_t k1 = k32[4*bit+1];
            const uint64_t k2 = k32[4*bit+2];
            const uint64_t k3 = k32[4*bit+3];

            /* Carry out the multiplication, term-by-term. Terms which
             * have no consistently detectible contribution to the
             * final result due to truncation are not included. */

            overflow = 0;
            sum= k3 * w2;
            term(k2 * w3);
            shift32();

            term(k3 * w1);
            term(k2 * w2);
            term(k1 * w3);
            w[3] = static_cast<uint32_t>(sum);
            shift32();

            if (bit == 25) {
                term(k3 * w0);
                term(k0 * w3);
            }
            term(k2 * w1);
            term(k1 * w2);
            w[2] = static_cast<uint32_t>(sum);
            shift32();

            if (bit == 25) {
                term(k2 * w0);
                term(k0 * w2);
            }
            term(k1 * w1);
            w[1] = static_cast<uint32_t>(sum);
            shift32();

            if (bit == 25) {
                term(k1 * w0);
                term(k0 * w1);
            }
            w[0] = static_cast<uint32_t>(sum);

            /* The above calculation can only possibly overflow on the
             * very last run through the loop. If there was overflow
             * the output would necessarily exceed MAX_MONEY and be
             * clamped so there is no need to proceed further. */
            if (bit == 25) {
                shift32();
                if (sum || w0) {
                    return kOverflow;
                }
            }
        }
    }

    /* Now we multiply the original value by the inverse demurrage
     * factor, in much the same way the fixed point calculations were
     * performed above, but with fewer terms since value has no
     * fractional component. */
    const uint64_t v0 = value >> 32;
    const uint64_t v1 = static_cast<uint32_t>(value);

    overflow = 0;
    sum = (v1 * w[3]) >> 32;

    term(v1 * w[2]);
    term(v0 * w[3]);
    shift32();

    term(v1 * w[1]);
    term(v0 * w[2]);
    const uint64_t r1 = static_cast<uint32_t>(sum);
    shift32();

    term(v1 * w[0]);
    term(v0 * w[1]);
    const uint64_t r0 = static_cast<uint32_t>(sum);
    shift32();

    #undef term
    #undef shift32

    /* The final term represents bits 65-128. If this term is
     * non-zero, or if shift32 left any residual value or overflow, we
     * know we have exceeded our range. */
    if (sum || (v0 && w[0])) {
        return kOverflow;
    }

    /* Finally we return our calculated result, clamped to never be
     * more than MAX_MONEY. */
    int64_t result = (static_cast<int64_t>(r0) << 32) | static_cast<int64_t>(r1);
    if (result > MAX_MONEY) {
        return kOverflow;
    }

    return sign * result;
}

int64_t GetTimeAdjustedValue(int64_t initial_value, int relative_depth)
{
    if (relative_depth < 0)
        return TimeAdjustValueReverse(initial_value, std::abs(relative_depth));
    else
        return TimeAdjustValueForward(initial_value, relative_depth);
}

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

void COutPoint::print() const
{
    LogPrintf("%s\n", ToString());
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, unsigned int nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn, unsigned int nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else
        str += strprintf(", scriptSig=%s", scriptSig.ToString().substr(0,24));
    if (nSequence != std::numeric_limits<unsigned int>::max())
        str += strprintf(", nSequence=%u", nSequence);
    str += ")";
    return str;
}

void CTxIn::print() const
{
    LogPrintf("%s\n", ToString());
}

CTxOut::CTxOut(int64_t nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}

uint256 CTxOut::GetHash() const
{
    return SerializeHash(*this);
}

std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, scriptPubKey.ToString().substr(0,30));
}

void CTxOut::print() const
{
    LogPrintf("%s\n", ToString());
}

uint256 CTransaction::GetHash() const
{
    return SerializeHash(*this);
}

bool CTransaction::IsNewerThan(const CTransaction& old) const
{
    if (vin.size() != old.vin.size())
        return false;
    for (unsigned int i = 0; i < vin.size(); i++)
        if (vin[i].prevout != old.vin[i].prevout)
            return false;

    bool fNewer = false;
    unsigned int nLowest = std::numeric_limits<unsigned int>::max();
    for (unsigned int i = 0; i < vin.size(); i++)
    {
        if (vin[i].nSequence != old.vin[i].nSequence)
        {
            if (vin[i].nSequence <= nLowest)
            {
                fNewer = false;
                nLowest = vin[i].nSequence;
            }
            if (old.vin[i].nSequence < nLowest)
            {
                fNewer = true;
                nLowest = old.vin[i].nSequence;
            }
        }
    }
    return fNewer;
}

int64_t CTransaction::GetPresentValueOfOutput(int n, int height) const
{
    if (height < refheight)
        throw std::runtime_error("CTransaction::GetPresentValueOfOutput() : destination height less than origin");
    return GetTimeAdjustedValue(vout[n].nValue, height-refheight);
}

int64_t CTransaction::GetValueOut() const
{
    int64_t nValueOut = 0;
    BOOST_FOREACH(const CTxOut& txout, vout)
    {
        nValueOut += txout.nValue;
        if (!MoneyRange(txout.nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error("CTransaction::GetValueOut() : value out of range");
    }
    return nValueOut;
}

double CTransaction::ComputePriority(double dPriorityInputs, unsigned int nTxSize) const
{
    // In order to avoid disincentivizing cleaning up the UTXO set we don't count
    // the constant overhead for each txin and up to 110 bytes of scriptSig (which
    // is enough to cover a compressed pubkey p2sh redemption) for priority.
    // Providing any more cleanup incentive than making additional inputs free would
    // risk encouraging people to create junk outputs to redeem later.
    if (nTxSize == 0)
        nTxSize = ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION);
    BOOST_FOREACH(const CTxIn& txin, vin)
    {
        unsigned int offset = 41U + std::min(110U, (unsigned int)txin.scriptSig.size());
        if (nTxSize > offset)
            nTxSize -= offset;
    }
    if (nTxSize == 0) return 0.0;
    return dPriorityInputs / nTxSize;
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%u, refheight=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        vin.size(),
        vout.size(),
        nLockTime,
        refheight);
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].ToString() + "\n";
    for (unsigned int i = 0; i < vout.size(); i++)
        str += "    " + vout[i].ToString() + "\n";
    return str;
}

void CTransaction::print() const
{
    LogPrintf("%s", ToString());
}

// Amount compression:
// * If the amount is 0, output 0
// * first, divide the amount (in base units) by the largest power of 10 possible; call the exponent e (e is max 9)
// * if e<9, the last digit of the resulting number cannot be 0; store it as d, and drop it (divide by 10)
//   * call the result n
//   * output 1 + 10*(9*n + d - 1) + e
// * if e==9, we only know the resulting number is not zero, so output 1 + 10*(n - 1) + 9
// (this is decodable, as d is in [1-9] and e is in [0-9])

uint64_t CTxOutCompressor::CompressAmount(uint64_t n)
{
    if (n == 0)
        return 0;
    int e = 0;
    while (((n % 10) == 0) && e < 9) {
        n /= 10;
        e++;
    }
    if (e < 9) {
        int d = (n % 10);
        assert(d >= 1 && d <= 9);
        n /= 10;
        return 1 + (n*9 + d - 1)*10 + e;
    } else {
        return 1 + (n - 1)*10 + 9;
    }
}

uint64_t CTxOutCompressor::DecompressAmount(uint64_t x)
{
    // x = 0  OR  x = 1+10*(9*n + d - 1) + e  OR  x = 1+10*(n - 1) + 9
    if (x == 0)
        return 0;
    x--;
    // x = 10*(9*n + d - 1) + e
    int e = x % 10;
    x /= 10;
    uint64_t n = 0;
    if (e < 9) {
        // x = 9*n + d - 1
        int d = (x % 9) + 1;
        x /= 9;
        // x = n
        n = x*10 + d;
    } else {
        n = x+1;
    }
    while (e) {
        n *= 10;
        e--;
    }
    return n;
}

uint256 CBlockHeader::GetHash() const
{
    return Hash(BEGIN(nVersion), END(nNonce));
}

uint256 CBlock::BuildMerkleTree() const
{
    vMerkleTree.clear();
    BOOST_FOREACH(const CTransaction& tx, vtx)
        vMerkleTree.push_back(tx.GetHash());
    int j = 0;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
                                       BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
        }
        j += nSize;
    }
    return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
}

std::vector<uint256> CBlock::GetMerkleBranch(int nIndex) const
{
    if (vMerkleTree.empty())
        BuildMerkleTree();
    std::vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        int i = std::min(nIndex^1, nSize-1);
        vMerkleBranch.push_back(vMerkleTree[j+i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

uint256 CBlock::CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
{
    if (nIndex == -1)
        return 0;
    BOOST_FOREACH(const uint256& otherside, vMerkleBranch)
    {
        if (nIndex & 1)
            hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
        nIndex >>= 1;
    }
    return hash;
}

void CBlock::print() const
{
    LogPrintf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        LogPrintf("  ");
        vtx[i].print();
    }
    LogPrintf("  vMerkleTree: ");
    for (unsigned int i = 0; i < vMerkleTree.size(); i++)
        LogPrintf("%s ", vMerkleTree[i].ToString());
    LogPrintf("\n");
}
