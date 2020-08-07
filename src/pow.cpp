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

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <primitives/block.h>
#include <uint256.h>

#include <array>

static const uint256 k_min_pow_limit = uint256S("0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

int64_t GetActualTimespan(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    // This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis.
    // Code courtesy of Art Forz
    int blocks_to_go_back = params.original_adjust_interval - 1;
    if ((pindexLast->nHeight+1) != params.original_adjust_interval)
        blocks_to_go_back = params.original_adjust_interval;

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - blocks_to_go_back;
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
}

std::pair<int64_t, int64_t> GetOriginalAdjustmentFactor(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    // Limit adjustment step
    int64_t actual_timespan = GetActualTimespan(pindexLast, params);
    if (actual_timespan < params.OriginalTargetTimespan()/4)
        actual_timespan = params.OriginalTargetTimespan()/4;
    if (actual_timespan > params.OriginalTargetTimespan()*4)
        actual_timespan = params.OriginalTargetTimespan()*4;

    return std::make_pair(params.OriginalTargetTimespan(), actual_timespan);
}

int64_t GetFilteredTime(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const std::size_t WINDOW = 144;
    static int32_t filter_coeff[WINDOW] = {
         -845859,  -459003,  -573589,  -703227,  -848199, -1008841,
        -1183669, -1372046, -1573247, -1787578, -2011503, -2243311,
        -2482346, -2723079, -2964681, -3202200, -3432186, -3650186,
        -3851924, -4032122, -4185340, -4306430, -4389146, -4427786,
        -4416716, -4349289, -4220031, -4022692, -3751740, -3401468,
        -2966915, -2443070, -1825548, -1110759,  -295281,   623307,
         1646668,  2775970,  4011152,  5351560,  6795424,  8340274,
         9982332, 11717130, 13539111, 15441640, 17417389, 19457954,
        21554056, 23695744, 25872220, 28072119, 30283431, 32493814,
        34690317, 36859911, 38989360, 41065293, 43074548, 45004087,
        46841170, 48573558, 50189545, 51678076, 53028839, 54232505,
        55280554, 56165609, 56881415, 57422788, 57785876, 57968085,
        57968084, 57785876, 57422788, 56881415, 56165609, 55280554,
        54232505, 53028839, 51678076, 50189545, 48573558, 46841170,
        45004087, 43074548, 41065293, 38989360, 36859911, 34690317,
        32493814, 30283431, 28072119, 25872220, 23695744, 21554057,
        19457953, 17417389, 15441640, 13539111, 11717130,  9982332,
         8340274,  6795424,  5351560,  4011152,  2775970,  1646668,
          623307,  -295281, -1110759, -1825548, -2443070, -2966915,
        -3401468, -3751740, -4022692, -4220031, -4349289, -4416715,
        -4427787, -4389146, -4306430, -4185340, -4032122, -3851924,
        -3650186, -3432186, -3202200, -2964681, -2723079, -2482346,
        -2243311, -2011503, -1787578, -1573247, -1372046, -1183669,
        -1008841,  -848199,  -703227,  -573589,  -459003,  -845858
    };

    int32_t time_delta[WINDOW];

    size_t idx = 0;
    const CBlockIndex *pitr = pindexLast;
    for (; idx!=WINDOW && pitr && pitr->pprev; ++idx, pitr=pitr->pprev)
        time_delta[idx] = static_cast<int32_t>(pitr->GetBlockTime() - pitr->pprev->GetBlockTime());
    for (; idx!=WINDOW; ++idx)
        time_delta[idx] = static_cast<int32_t>(params.nPowTargetSpacing);

    int64_t filtered_time = 0;
    for (idx=0; idx<WINDOW; ++idx)
        filtered_time += (int64_t)filter_coeff[idx] * (int64_t)time_delta[idx];

    return filtered_time;
}

std::pair<int64_t, int64_t> GetFilteredAdjustmentFactor(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const std::pair<int16_t, int16_t> gain(41, 400); //!< 0.1025
    const std::pair<int16_t, int16_t> limiter(211, 200); //!< 1.055

    int64_t filtered_time = GetFilteredTime(pindexLast, params);

    int64_t numerator;
    int64_t denominator;
    if (filtered_time < 597105209444LL) {
        numerator   = limiter.first;
        denominator = limiter.second;
    } else if (filtered_time > 1943831401459LL) {
        numerator   = limiter.second;
        denominator = limiter.first;
    } else {
        numerator = ((int64_t)(gain.first + gain.second) * params.nPowTargetSpacing) << 31;
        numerator -= (int64_t)gain.first * filtered_time;
        denominator = ((int64_t)gain.second * params.nPowTargetSpacing) << 31;
    }

    return std::make_pair(numerator, denominator);
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    static const unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Special, one-time adjustment due to the "hash crash" of Apr/May 2013
    // which rushed the introduction of the new difficulty adjustment filter.
    // We adjust back to the difficulty prior to the last adjustment.
    if (pindexLast->GetBlockHash() == uint256S("0x0000000000003bd73ea13954fbbf1cf50b5384f961d142a75a3dfe106f793a20"))
        return 0x1b01c13a;

    const bool use_filter = (pindexLast->nHeight >= (params.diff_adjust_threshold - 1));
    const int64_t interval = use_filter ? params.filtered_adjust_interval : params.original_adjust_interval;

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % interval != 0)
    {
        return pindexLast->nBits;
    }

    return CalculateNextWorkRequired(pindexLast, params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    const bool use_filter = (pindexLast->nHeight >= (params.diff_adjust_threshold - 1));

    std::pair<int64_t, int64_t> adjustment_factor;

    if (use_filter) {
        adjustment_factor = GetFilteredAdjustmentFactor(pindexLast, params);
    } else {
        adjustment_factor = GetOriginalAdjustmentFactor(pindexLast, params);
    }

    assert(adjustment_factor.first >= 0);
    assert(adjustment_factor.second > 0);

    // Retarget
    arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    arith_uint320 bnTmp(bnNew);
    bnTmp *= arith_uint320(static_cast<uint64_t>(adjustment_factor.second));
    bnTmp /= arith_uint320(static_cast<uint64_t>(adjustment_factor.first));
    assert(bnTmp.TruncateTo256(bnNew));

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

// Check that on difficulty adjustments, the new difficulty does not increase
// or decrease beyond the permitted limits.
bool PermittedDifficultyTransition(const Consensus::Params& params, int64_t height, uint32_t old_nbits, uint32_t new_nbits)
{
    if (params.fPowNoRetargeting) return true;

    if (height >= (params.diff_adjust_threshold - 1)) {
        return true;
    } else if (height % params.original_adjust_interval == 0) {
        int64_t smallest_timespan = params.OriginalTargetTimespan()/4;
        int64_t largest_timespan = params.OriginalTargetTimespan()*4;

        const arith_uint256 pow_limit = UintToArith256(params.powLimit);
        arith_uint256 observed_new_target;
        observed_new_target.SetCompact(new_nbits);

        // Calculate the largest difficulty value possible:
        arith_uint256 largest_difficulty_target;
        largest_difficulty_target.SetCompact(old_nbits);
        largest_difficulty_target *= largest_timespan;
        largest_difficulty_target /= params.OriginalTargetTimespan();

        if (largest_difficulty_target > pow_limit) {
            largest_difficulty_target = pow_limit;
        }

        // Round and then compare this new calculated value to what is
        // observed.
        arith_uint256 maximum_new_target;
        maximum_new_target.SetCompact(largest_difficulty_target.GetCompact());
        if (maximum_new_target < observed_new_target) return false;

        // Calculate the smallest difficulty value possible:
        arith_uint256 smallest_difficulty_target;
        smallest_difficulty_target.SetCompact(old_nbits);
        smallest_difficulty_target *= smallest_timespan;
        smallest_difficulty_target /= params.OriginalTargetTimespan();

        if (smallest_difficulty_target > pow_limit) {
            smallest_difficulty_target = pow_limit;
        }

        // Round and then compare this new calculated value to what is
        // observed.
        arith_uint256 minimum_new_target;
        minimum_new_target.SetCompact(smallest_difficulty_target.GetCompact());
        if (minimum_new_target > observed_new_target) return false;
    } else if (old_nbits != new_nbits) {
        return false;
    }
    return true;
}

int64_t GetFilteredTimeAux(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    // Unfortunately pretty much all digital control code looks like arcane
    // magic to nonpractitioners.  The below code implements a second-order
    // low-pass filter over observed inter-block intervals in order to estimate
    // the current block expectation time for the purpose of difficulty
    // adjustment.  Essentially `x` is an array of observed inter-block times
    // and `y` is an array of historical predictions, and the next predicted
    // value is calculated by:
    //
    //     y[0] = b0 * x[0] + b1 * x[1] + b2 * x[2]
    //                      - a1 * y[1] - a2 * y[2]
    //
    // where b0, b1, b2, a1, and a2 are constants, with the constraint
    //
    //     (b0 + b1 + b2) - (a1 + a2) = 1.0
    //
    // so that the weighted sum (the prediction) is a sort of average.  Since
    // the x's are themselves differences, a total of 4 block header timestamps
    // are required to calculate the necessary 3 difference values.  And since
    // prediction values are delayed a block, there are only two y values
    // available.
    //
    // The magic constants are Bessel/Thomson digital filter coefficients,
    // scaled to reflect the fact that we are operating on 32.32 fixed-point
    // numbers.  The values were calculated with the scipy library:
    //
    //     b, a = signal.bessel(2, 0.5, 'low')
    //
    // Note that for this choice of filter, the constant a1 ends up being zero.
    // Therefore this implementation actually implements:
    //
    //     y[0] = b0 * x[0] + b1 * x[1] + b2 * x[2]
    //                                  - a2 * y
    //
    // The input array of x values are integers, while y (or y[2] in the full
    // second-order equation above) is a 32.32 fixed-point number.
    std::array<int64_t, 3> x = { 0, 0, 0 };
    int64_t y = 0;

    auto pitr = pindexLast;
    for (size_t idx = 0; idx != x.size(); ++idx) {
        if (pitr && pitr->pprev) {
            x[idx] = pitr->GetBlockTime() - pitr->pprev->GetBlockTime();
            pitr = pitr->pprev;
        } else {
            x[idx] = params.aux_pow_target_spacing;
        }
    }

    if (pindexLast && pindexLast->pprev) {
        y = pindexLast->pprev->GetFilteredBlockTime();
    } else {
        y = params.aux_pow_target_spacing << 32;
    }

    static const int64_t low = 0xffffffff;
    auto time = x[0] * static_cast<int64_t>(0x4498517a)
              + x[1] * static_cast<int64_t>(0x8930a2f5)
              + x[2] * static_cast<int64_t>(0x4498517a)
         - (y >> 32) * static_cast<int64_t>(0x126145ea)
        - ((y & low) * static_cast<int64_t>(0x126145ea) >> 32);

    // The predicted time is stored inside a 56-bit field within the block
    // header, so its value needs to be clamped.  This restricts the predicted
    // value to be +/- 97 days, which is unlikely to be an issue in practice.
    if (time > 0x007fffffffffffffLL)
        time = 0x007fffffffffffffLL;
    if (time < -36028797018963968LL) // 0xffffffffffffff
        time = -36028797018963968LL;

    return time;
}

std::pair<int64_t, int64_t> GetAdjustmentFactorAux(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const std::pair<int16_t, int16_t> gain(1, 64); //!< 0.015625
    const std::pair<int16_t, int16_t> limiter(17, 16); //!< 1.0625

    int64_t filtered_time = GetFilteredTimeAux(pindexLast, params);

    int64_t numerator;
    int64_t denominator;
    if (filtered_time < -11596411699199LL) { // -2700 sec
        // Any lower and we are limited in the adjustment upward.
        numerator   = limiter.first;
        denominator = limiter.second;
    } else if (filtered_time > 18417830345788LL) { // approx. 4288 sec
        // Any higher and we are limited in the adjustment downward.
        numerator   = limiter.second;
        denominator = limiter.first;
    } else {
        // The following computes the value
        //
        //   f = 1 - G * (actual - expected) / expected
        //
        // where f is the desired adjustment factor, actual is the
        // filter-estimated inter-block interval (in seconds), spacing is the
        // target interval (in seconds), and G is the gain constant: 0.015625.
        //
        // Since we are computing using integers, the terms have been rearranged
        // algebraically to prevent truncation error and deal with filtered_time
        // as a 32.32 fixed-point number.
        numerator = ((int64_t)(gain.first + gain.second) * params.aux_pow_target_spacing) << 32;
        numerator -= (int64_t)gain.first * filtered_time;
        denominator = ((int64_t)gain.second * params.aux_pow_target_spacing) << 32;
    }

    return std::make_pair(numerator, denominator);
}

uint32_t GetNextWorkRequiredAux(const CBlockIndex* pindexLast, const CBlockHeader& block, const Consensus::Params& params)
{
    // The very first block to have an auxiliary proof-of-work starts
    // with 50% the native difficulty.
    if (!pindexLast || pindexLast->m_aux_pow.IsNull()) {
        arith_uint256 target;
        target.SetCompact(block.nBits);
        target <<= 1;
        return target.GetCompact();
    }

    return CalculateNextWorkRequiredAux(pindexLast, params);
}

uint32_t CalculateNextWorkRequiredAux(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    arith_uint256 pow_limit = UintToArith256(params.aux_pow_limit);

    if (params.fPowNoRetargeting) {
        return pindexLast->m_aux_pow.m_commit_bits;
    }

    std::pair<int64_t, int64_t> adjustment_factor;

    adjustment_factor = GetAdjustmentFactorAux(pindexLast, params);

    assert(adjustment_factor.first >= 0);
    assert(adjustment_factor.second > 0);

    // Retarget
    arith_uint256 new_target;
    new_target.SetCompact(pindexLast->m_aux_pow.m_commit_bits);
    arith_uint320 tmp(new_target);
    tmp *= arith_uint320(static_cast<uint64_t>(adjustment_factor.second));
    tmp /= arith_uint320(static_cast<uint64_t>(adjustment_factor.first));
    assert(tmp.TruncateTo256(new_target));

    return ((new_target > pow_limit) ? pow_limit : new_target).GetCompact();
}

// Called after activation of the size-expansion rule changes, at
// which time the difficulty adjustment becomes largely unchecked.
// For DoS prevention purposes we require that the difficulty adjust
// by no more than +/- 2x as compared with the difficulties of the
// last 11 blocks. This is enough of a constraint that any DoS attack
// is forced to have non-trivial mining costs (e.g. equal to extending
// the tip by 6 blocks to reduce difficulty by more than a half, work
// equal to extending the tip by 9 blocks to reduce by more than a
// quarter, 10.5 times present difficulty to reduce by more than an
// eigth, etc. To reduce to arbitrary levels requires a dozen blocks
// worth of work at the difficulty of the last valid block.
bool CheckNextWorkRequiredAux(const CBlockIndex* pindexLast, const CBlockHeader& block, const Consensus::Params& params)
{
    // There's nothing to check before activation of merged mining.
    if (block.m_aux_pow.IsNull()) {
        return true;
    }

    // Should never happen...
    // (The genesis block should have already been handled above.)
    if (!pindexLast) {
        return false;
    }

    // Special case for the first merge-mined block.
    if (pindexLast->m_aux_pow.IsNull()) {
        return (block.m_aux_pow.m_commit_bits == UintToArith256(params.aux_pow_limit).GetCompact());
    }

    // If look reversed, that is to be expected. We set min to the largest
    // possible value, and max to the smallest. That way these will be replaced
    // with actual block values as we loop through the past 11 blocks.
    arith_uint256 min = UintToArith256(params.aux_pow_limit);
    arith_uint256 max = arith_uint256(1);

    // After this loop, min will be half the largest work target of
    // the past 11 blocks, and max will be twice the smallest.
    for (int i = 0; i < 11 && pindexLast && !pindexLast->m_aux_pow.IsNull(); ++i, pindexLast = pindexLast->pprev) {
        arith_uint256 target;
        target.SetCompact(pindexLast->m_aux_pow.m_commit_bits);
        arith_uint256 local_min = target >> 1;
        arith_uint256 local_max = target << 1;
        if (min > local_min) {
            min = local_min;
        }
        if (max < local_max) {
            max = local_max;
        }
    }

    // See if the passed in block's m_commit_bits specifies a target within the
    // range of half to twice the work targets of the past 11 blocks, inclusive
    // of the endpoints.
    arith_uint256 target;
    target.SetCompact(block.m_aux_pow.m_commit_bits);
    return ((min <= target) && (target <= max));
}

bool CheckAuxiliaryProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    bool mutated = false;
    bool negative = false;
    bool overflow = false;
    arith_uint256 target;

    if (block.m_aux_pow.IsNull()) {
        return true;
    }

    // Calculate the auxiliary proof-of-work, which is a block header of the
    // parent chain, presumably bitcoin.  Since this involves a Merkle root
    // calculation, there's a possibility the data is in non-canonical form,
    // and we reject that as a block data mutation.
    auto aux_hash = block.GetAuxiliaryHash(params, &mutated);
    if (mutated) {
        return false;
    }

    // Calculate the target value for the auxiliary proof-of-work, using
    // the nBits value in our block header, not the auxiliary block.
    target.SetCompact(block.m_aux_pow.m_commit_bits, &negative, &overflow);
    if (negative || target == 0 || overflow) {
        return false;
    }

    // Offset by the bias value.
    unsigned char bias = block.GetBias();
    if ((256 - target.bits()) < bias) {
        return false;
    }
    target <<= bias;

    // Check auxiliary proof-of-work (1st stage)
    if (UintToArith256(aux_hash.first) > target) {
        return false;
    }

    // Calculate target for 2nd stage.
    target = UintToArith256(uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    target >>= bias;

    // Check auxiliary proof-of-work (2nd stage)
    if (UintToArith256(aux_hash.second) > target) {
        return false;
    }

    return true;
}

bool CheckProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    bool negative = false;
    bool overflow = false;
    arith_uint256 target;

    // Calculate the compatibility proof-of-work.
    uint256 hash = block.GetHash();

    target.SetCompact(block.nBits, &negative, &overflow);

    // Check range
    if (negative || target == 0 || overflow || target > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > target)
        return false;

    return true;
}
