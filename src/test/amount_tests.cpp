// Copyright (c) 2016-2021 The Bitcoin Core developers
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

#include <consensus/amount.h>
#include <policy/feerate.h>

#include <limits>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(amount_tests)

BOOST_AUTO_TEST_CASE(MoneyRangeTest)
{
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(-1)), false);
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(0)), true);
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(1)), true);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY), true);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY + CAmount(1)), false);
}

BOOST_AUTO_TEST_CASE(GetFeeTest)
{
    CFeeRate feeRate, altFeeRate;

    feeRate = CFeeRate(0);
    // Must always return 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e5), CAmount(0));

    feeRate = CFeeRate(1000);
    // Must always just return the arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), CAmount(1));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), CAmount(121));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), CAmount(999));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), CAmount(1e3));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), CAmount(9e3));

    feeRate = CFeeRate(-1000);
    // Must always just return -1 * arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), CAmount(-1));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), CAmount(-121));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), CAmount(-999));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), CAmount(-1e3));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), CAmount(-9e3));

    feeRate = CFeeRate(123);
    // Rounds up the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), CAmount(1)); // Special case: returns 1 instead of 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), CAmount(2));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), CAmount(15));
    BOOST_CHECK_EQUAL(feeRate.GetFee(122), CAmount(16));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), CAmount(123));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), CAmount(123));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), CAmount(1107));

    feeRate = CFeeRate(-123);
    // Truncates the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), CAmount(-1)); // Special case: returns -1 instead of 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), CAmount(-1));

    // check alternate constructor
    feeRate = CFeeRate(1000);
    altFeeRate = CFeeRate(feeRate);
    BOOST_CHECK_EQUAL(feeRate.GetFee(100), altFeeRate.GetFee(100));

    // Check full constructor
    BOOST_CHECK(CFeeRate(CAmount(-1), 0) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(0), 0) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(1), 0) == CFeeRate(0));
    // default value
    BOOST_CHECK(CFeeRate(CAmount(-1), 1000) == CFeeRate(-1));
    BOOST_CHECK(CFeeRate(CAmount(0), 1000) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(1), 1000) == CFeeRate(1));
    // lost precision (can only resolve kria per kB)
    BOOST_CHECK(CFeeRate(CAmount(1), 1001) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(2), 1001) == CFeeRate(1));
    // some more integer checks
    BOOST_CHECK(CFeeRate(CAmount(26), 789) == CFeeRate(32));
    BOOST_CHECK(CFeeRate(CAmount(27), 789) == CFeeRate(34));
    // Maximum size in bytes, should not crash
    CFeeRate(MAX_MONEY, std::numeric_limits<uint32_t>::max()).GetFeePerK();

    // check multiplication operator
    // check multiplying by zero
    feeRate = CFeeRate(1000);
    BOOST_CHECK(0 * feeRate == CFeeRate(0));
    BOOST_CHECK(feeRate * 0 == CFeeRate(0));
    // check multiplying by a positive integer
    BOOST_CHECK(3 * feeRate == CFeeRate(3000));
    BOOST_CHECK(feeRate * 3 == CFeeRate(3000));
    // check multiplying by a negative integer
    BOOST_CHECK(-3 * feeRate == CFeeRate(-3000));
    BOOST_CHECK(feeRate * -3 == CFeeRate(-3000));
    // check commutativity
    BOOST_CHECK(2 * feeRate == feeRate * 2);
    // check with large numbers
    int largeNumber = 1000000;
    BOOST_CHECK(largeNumber * feeRate == feeRate * largeNumber);
    // check boundary values
    int maxInt = std::numeric_limits<int>::max();
    feeRate = CFeeRate(maxInt);
    BOOST_CHECK(feeRate * 2 == CFeeRate(static_cast<int64_t>(maxInt) * 2));
    BOOST_CHECK(2 * feeRate == CFeeRate(static_cast<int64_t>(maxInt) * 2));
    // check with zero fee rate
    feeRate = CFeeRate(0);
    BOOST_CHECK(feeRate * 5 == CFeeRate(0));
    BOOST_CHECK(5 * feeRate == CFeeRate(0));
}

BOOST_AUTO_TEST_CASE(BinaryOperatorTest)
{
    CFeeRate a, b;
    a = CFeeRate(1);
    b = CFeeRate(2);
    BOOST_CHECK(a < b);
    BOOST_CHECK(b > a);
    BOOST_CHECK(a == a);
    BOOST_CHECK(a <= b);
    BOOST_CHECK(a <= a);
    BOOST_CHECK(b >= a);
    BOOST_CHECK(b >= b);
    // a should be 0.00000002 FRC/kvB now
    a += a;
    BOOST_CHECK(a == b);
}

BOOST_AUTO_TEST_CASE(ToStringTest)
{
    CFeeRate feeRate;
    feeRate = CFeeRate(1);
    BOOST_CHECK_EQUAL(feeRate.ToString(), "0.00000001 FRC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeEstimateMode::FRC_KVB), "0.00000001 FRC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeEstimateMode::SAT_VB), "0.001 sat/vB");
}

/* The demurrage time-adjustment functions are consensus critical: any
 * change to their output for any input is a hard fork. The tests below
 * therefore pin down both the trivial domain edges and a matrix of
 * exact input/output pairs, so that an accidental behavioral change
 * cannot slip through. See issue #17. */

BOOST_AUTO_TEST_CASE(TimeAdjustTrivialTest)
{
    const bool old_disable_time_adjust = disable_time_adjust;
    disable_time_adjust = false;

    /* A distance of zero is the identity in both directions. */
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(0, 0), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(1, 0), 1);
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(COIN, 0), COIN);
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(MAX_MONEY, 0), MAX_MONEY);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(0, 0), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(1, 0), 1);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(COIN, 0), COIN);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(MAX_MONEY, 0), MAX_MONEY);

    /* Zero is a fixed point at any distance, including past the
     * saturation threshold. */
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(0, 1), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(0, 52560), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(0, uint32_t{1} << 26), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(0, 1), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(0, 52560), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(0, uint32_t{1} << 26), 0);

    /* A distance of 2^26 or more decays any amount to nothing going
     * forward, and going in reverse would make even a single kria
     * exceed MAX_MONEY, so the result saturates. */
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(1, uint32_t{1} << 26), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(MAX_MONEY, uint32_t{1} << 26), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(MAX_MONEY, std::numeric_limits<uint32_t>::max()), 0);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(1, uint32_t{1} << 26), MAX_MONEY);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(-1, uint32_t{1} << 26), -MAX_MONEY);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(MAX_MONEY, std::numeric_limits<uint32_t>::max()), MAX_MONEY);

    /* Reverse adjustment saturates at MAX_MONEY rather than
     * overflowing, even at the smallest distance. */
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(MAX_MONEY, 1), MAX_MONEY);

    /* GetTimeAdjustedValue() dispatches on the sign of the relative
     * depth. */
    BOOST_CHECK_EQUAL(GetTimeAdjustedValue(COIN, 0), COIN);
    BOOST_CHECK_EQUAL(GetTimeAdjustedValue(COIN, 52560), TimeAdjustValueForward(COIN, 52560));
    BOOST_CHECK_EQUAL(GetTimeAdjustedValue(COIN, -52560), TimeAdjustValueReverse(COIN, 52560));

    /* With time adjustment disabled (bitcoin unit test compatibility
     * mode), both functions are the identity regardless of distance. */
    disable_time_adjust = true;
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(COIN, 52560), COIN);
    BOOST_CHECK_EQUAL(TimeAdjustValueForward(MAX_MONEY, uint32_t{1} << 26), MAX_MONEY);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(COIN, 52560), COIN);
    BOOST_CHECK_EQUAL(TimeAdjustValueReverse(MAX_MONEY, uint32_t{1} << 26), MAX_MONEY);

    disable_time_adjust = old_disable_time_adjust;
}

BOOST_AUTO_TEST_CASE(TimeAdjustOneBlockTest)
{
    const bool old_disable_time_adjust = disable_time_adjust;
    disable_time_adjust = false;

    /* Over a single block the demurrage rate (1 - 2^-20) is exactly
     * representable in 64-bit fixed point, so the forward calculation
     * has a closed form: the value loses its 2^-20 fraction, rounded
     * up (equivalently, the result is rounded down). */
    const CAmount vectors[] = {1, 2, 999, (CAmount{1} << 20) - 1, CAmount{1} << 20, (CAmount{1} << 20) + 1, COIN, 7 * COIN + 123, MAX_MONEY};
    for (const CAmount& value : vectors) {
        const CAmount expected = value - (value + (CAmount{1} << 20) - 1) / (CAmount{1} << 20);
        BOOST_CHECK_EQUAL(TimeAdjustValueForward(value, 1), expected);
        BOOST_CHECK_EQUAL(TimeAdjustValueForward(-value, 1), -expected);
    }

    disable_time_adjust = old_disable_time_adjust;
}

BOOST_AUTO_TEST_CASE(TimeAdjustGoldenValuesTest)
{
    const bool old_disable_time_adjust = disable_time_adjust;
    disable_time_adjust = false;

    /* Exact input/output pairs for both directions of adjustment.
     *
     * The distances cover single blocks, one year of blocks (52560),
     * the 1/e point (2^20 blocks, at which the compounded rate is
     * (1 - 2^-20)^(2^20) ~= e^-1), a single high ladder entry (2^22),
     * and 2^26 - 1, which exercises every entry of the exponentiation
     * ladder at once. The expected outputs were generated from an
     * independent reimplementation of the fixed-point algorithms and
     * cross-checked against 80-digit precision real arithmetic, from
     * which they never deviate by more than the documented
     * round-toward-zero truncation. */
    struct TestVector {
        CAmount value;
        uint32_t distance;
        CAmount forward;
        CAmount reverse;
    };
    const TestVector vectors[] = {
        {1LL, 1, 0LL, 1LL},
        {1LL, 2, 0LL, 1LL},
        {1LL, 3, 0LL, 1LL},
        {1LL, 1000, 0LL, 1LL},
        {1LL, 52560, 0LL, 1LL},
        {1LL, 1048576, 0LL, 2LL},
        {1LL, 4194304, 0LL, 54LL},
        {1LL, 67108863, 0LL, 9007199254740991LL},
        {2LL, 1, 1LL, 2LL},
        {2LL, 2, 1LL, 2LL},
        {2LL, 3, 1LL, 2LL},
        {2LL, 1000, 1LL, 2LL},
        {2LL, 52560, 1LL, 2LL},
        {2LL, 1048576, 0LL, 5LL},
        {2LL, 4194304, 0LL, 109LL},
        {2LL, 67108863, 0LL, 9007199254740991LL},
        {1000LL, 1, 999LL, 1000LL},
        {1000LL, 2, 999LL, 1000LL},
        {1000LL, 3, 999LL, 1000LL},
        {1000LL, 1000, 999LL, 1000LL},
        {1000LL, 52560, 951LL, 1051LL},
        {1000LL, 1048576, 367LL, 2718LL},
        {1000LL, 4194304, 18LL, 54598LL},
        {1000LL, 67108863, 0LL, 9007199254740991LL},
        {100000000LL, 1, 99999904LL, 100000095LL},
        {100000000LL, 2, 99999809LL, 100000190LL},
        {100000000LL, 3, 99999713LL, 100000286LL},
        {100000000LL, 1000, 99904677LL, 100095412LL},
        {100000000LL, 52560, 95111038LL, 105140266LL},
        {100000000LL, 1048576, 36787926LL, 271828312LL},
        {100000000LL, 4194304, 1831560LL, 5459825417LL},
        {100000000LL, 67108863, 0LL, 9007199254740991LL},
        {1234567890123456LL, 1, 1234566712747767LL, 1234569067500267LL},
        {1234567890123456LL, 2, 1234565535373201LL, 1234570244878201LL},
        {1234567890123456LL, 3, 1234564357999758LL, 1234571422257259LL},
        {1234567890123456LL, 1000, 1233391075111954LL, 1235745827969068LL},
        {1234567890123456LL, 52560, 1174210346738858LL, 1298027972208671LL},
        {1234567890123456LL, 1048576, 454171928940582LL, 3355905061942482LL},
        {1234567890123456LL, 4194304, 22611856530365LL, 9007199254740991LL},
        {1234567890123456LL, 67108863, 0LL, 9007199254740991LL},
        {9007199254740991LL, 1, 9007190664806399LL, 9007199254740991LL},
        {9007199254740991LL, 2, 9007182074879999LL, 9007199254740991LL},
        {9007199254740991LL, 3, 9007173484961790LL, 9007199254740991LL},
        {9007199254740991LL, 1000, 8998613410755119LL, 9007199254740991LL},
        {9007199254740991LL, 52560, 8566840790746455LL, 9007199254740991LL},
        {9007199254740991LL, 1048576, 3313561848323150LL, 9007199254740991LL},
        {9007199254740991LL, 4194304, 164972294288531LL, 9007199254740991LL},
        {9007199254740991LL, 67108863, 0LL, 9007199254740991LL},
    };

    for (const TestVector& test : vectors) {
        /* The pinned values themselves. */
        BOOST_CHECK_EQUAL(TimeAdjustValueForward(test.value, test.distance), test.forward);
        BOOST_CHECK_EQUAL(TimeAdjustValueReverse(test.value, test.distance), test.reverse);

        /* Negation commutes with adjustment in either direction. */
        BOOST_CHECK_EQUAL(TimeAdjustValueForward(-test.value, test.distance), -test.forward);
        BOOST_CHECK_EQUAL(TimeAdjustValueReverse(-test.value, test.distance), -test.reverse);

        /* GetTimeAdjustedValue() is a sign-dispatched wrapper. */
        BOOST_CHECK_EQUAL(GetTimeAdjustedValue(test.value, static_cast<int>(test.distance)), test.forward);
        BOOST_CHECK_EQUAL(GetTimeAdjustedValue(test.value, -static_cast<int>(test.distance)), test.reverse);

        /* Demurrage only ever shrinks a value going forward and grows
         * it going in reverse. */
        BOOST_CHECK(test.forward <= test.value);
        BOOST_CHECK(test.reverse >= test.value);
    }

    disable_time_adjust = old_disable_time_adjust;
}

BOOST_AUTO_TEST_CASE(TimeAdjustRoundTripTest)
{
    const bool old_disable_time_adjust = disable_time_adjust;
    disable_time_adjust = false;

    /* Because both directions round toward zero, a forward-then-
     * reverse round trip is not the exact identity; it loses at most
     * a few kria to truncation. These exact pinned values demonstrate
     * (and lock in) that the round-trip error is small and always
     * downward. */
    struct TestVector {
        CAmount value;
        uint32_t distance;
        CAmount round_trip;
    };
    const TestVector vectors[] = {
        {1000LL, 52560, 999LL},
        {100000000LL, 52560, 99999999LL},
        {100000000LL, 1048576, 99999998LL},
        {1234567890123456LL, 1000, 1234567890123455LL},
        {9007199254740991LL, 1048576, 9007199254740988LL},
    };
    for (const TestVector& test : vectors) {
        const CAmount rt = TimeAdjustValueReverse(TimeAdjustValueForward(test.value, test.distance), test.distance);
        BOOST_CHECK_EQUAL(rt, test.round_trip);
        BOOST_CHECK(rt <= test.value);
    }

    disable_time_adjust = old_disable_time_adjust;
}

BOOST_AUTO_TEST_CASE(ScripConversionTest)
{
    const bool old_disable_time_adjust = disable_time_adjust;
    disable_time_adjust = false;

    /* Scrip values are freicoin amounts expressed in the zero-
     * demurrage epoch of block height 5040000 (SCRIP_EPOCH in
     * consensus/amount.cpp). At exactly the epoch height the
     * conversion is the identity. */
    BOOST_CHECK_EQUAL(FreicoinToScrip(0, 5040000), 0);
    BOOST_CHECK_EQUAL(FreicoinToScrip(COIN, 5040000), COIN);
    BOOST_CHECK_EQUAL(FreicoinToScrip(MAX_MONEY, 5040000), MAX_MONEY);
    BOOST_CHECK_EQUAL(ScripToFreicoin(0, 5040000), 0);
    BOOST_CHECK_EQUAL(ScripToFreicoin(COIN, 5040000), COIN);
    BOOST_CHECK_EQUAL(ScripToFreicoin(MAX_MONEY, 5040000), MAX_MONEY);

    /* One year (52560 blocks) before the epoch, a present-value coin
     * is worth less by the time the epoch arrives, so its scrip
     * representation is the forward-adjusted amount; one year after
     * the epoch the adjustment runs in reverse. ScripToFreicoin
     * applies the opposite adjustment of FreicoinToScrip at the same
     * height. */
    BOOST_CHECK_EQUAL(FreicoinToScrip(COIN, 5040000 - 52560), 95111038LL);
    BOOST_CHECK_EQUAL(ScripToFreicoin(95111038LL, 5040000 - 52560), 99999999LL);
    BOOST_CHECK_EQUAL(FreicoinToScrip(COIN, 5040000 + 52560), 105140266LL);
    BOOST_CHECK_EQUAL(ScripToFreicoin(105140266LL, 5040000 + 52560), 99999999LL);

    disable_time_adjust = old_disable_time_adjust;
}

BOOST_AUTO_TEST_SUITE_END()
