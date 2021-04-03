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

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"

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
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    const bool use_filter = (pindexLast->nHeight >= (params.diff_adjust_threshold - 1));
    const int64_t interval = use_filter ? params.filtered_adjust_interval : params.original_adjust_interval;

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % interval != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % interval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
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
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
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

bool CheckProofOfWork(uint256 hash, unsigned int nBits, unsigned char bias, const Consensus::Params& params)
{
    bool fNegative = false;
    bool fOverflow = false;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    if (bias) {
        fOverflow = fOverflow || (bias > (256 - bnTarget.bits()));
        bnTarget <<= bias;
    }

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
