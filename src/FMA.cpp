#include "FMA.hpp"
#include <systemc>
#include <cstdint>

using namespace sc_core;

FMA::FMA(sc_module_name nm, uint32_t mant_bits, uint32_t exp_bits, int rm)
    : sc_module(nm), mantissa_bits(mant_bits), exponent_bits(exp_bits), bias((1 << (exp_bits - 1)) - 1), round_mode(rm)
{
    SC_METHOD(run);
    sensitive << r1 << r2 << r3;
}

void FMA::run()
{
    clearFlags();
    // extracting e1 , m1 ,s1 ...
    extractAll();
    // this is a chosen standard nan value that would be the result of every nan propagation
    uint32_t nan_constant = (((1 << exponent_bits) - 1) << mantissa_bits) + 1;
    bool is_r1_zero = (r1.read() & 0x7FFFFFFF) == 0;
    bool is_r2_zero = (r2.read() & 0x7FFFFFFF) == 0;
    bool is_r3_zero = (r3.read() & 0x7FFFFFFF) == 0;
    // handling of all nan and inf special cases
    if (is_nan(e1, exponent_bits, m1) || is_nan(e2, exponent_bits, m2) || is_nan(e3, exponent_bits, m3))
    {
        nan.write(true);
        ro.write(nan_constant);
        return;
    }
    // checking if mul part itself gives a nan
    if ((is_inf(e2, exponent_bits, m2) && is_r3_zero) || (is_inf(e3, exponent_bits, m3) && is_r2_zero))
    {
        nan.write(true);
        ro.write(nan_constant);
        return;
    }
    // now checking inf combinations with the adder
    if (is_inf(e2, exponent_bits, m2) || is_inf(e3, exponent_bits, m3))
    {
        sign_product = s2 ^ s3;
        if (is_r1_zero)
        {
            inexact.write(true);
            // inf based on sign
            if (sign_product == 1)
            {
                ro.write(negative_inf_constant);
                sign.write(1);
            }
            else
            {
                ro.write(positive_inf_constant);
                sign.write(0);
            }
            return;
        }
        if (is_inf(e1, exponent_bits, m1) && s1 != sign_product)
        {
            nan.write(true);
            ro.write(nan_constant);
        }
        else
        {
            inexact.write(true);
            // inf based on sign
            if (sign_product == 1)
            {
                ro.write(negative_inf_constant);
                sign.write(1);
            }
            else
            {
                ro.write(positive_inf_constant);
                sign.write(0);
            }
            return;
        }
        return;
    }
    // if result of mul part is zero we just return whatever r1 is
    if (is_r2_zero || is_r3_zero)
    {
        ro.write(r1.read());
        if ((r1.read() & 0x7FFFFFFF) == 0)
        {
            zero.write(true);
        }
        sign.write(s1);
        return;
    }
    // adding leading 1. when necessary and filling up real_e values
    buildParts();
    // multiplyExact is true when overflow/underflow is already handled and hence we don't need to continue
    if (multiplyExact())
    {
        return;
    }
    // allign leading 1. of adder with leading 1. of mul result
    alignAddend();
    // this will take the work of adding/subbing and rounding
    main_funct();
}

void FMA::clearFlags()
{
    zero.write(false);
    sign.write(false);
    overflow.write(false);
    underflow.write(false);
    inexact.write(false);
    nan.write(false);
}

void FMA::extractAll()
{
    uint32_t rawC = r1.read();
    uint32_t rawA = r2.read();
    uint32_t rawB = r3.read();

    s1 = (rawC >> 31) & 1;
    e1 = (rawC >> mantissa_bits) & ((1u << exponent_bits) - 1);
    m1 = rawC & ((1u << mantissa_bits) - 1);

    s2 = (rawA >> 31) & 1;
    e2 = (rawA >> mantissa_bits) & ((1u << exponent_bits) - 1);
    m2 = rawA & ((1u << mantissa_bits) - 1);

    s3 = (rawB >> 31) & 1;
    e3 = (rawB >> mantissa_bits) & ((1u << exponent_bits) - 1);
    m3 = rawB & ((1u << mantissa_bits) - 1);
    positive_inf_constant = ((1 << exponent_bits) - 1) << mantissa_bits;
    negative_inf_constant = (1 << 31) | (positive_inf_constant);
}

void FMA::buildParts()
{
    // necessary since m1 ==0 and e1==0 would notify us that r1 is actually 1 , we do not want to add 1.
    if (!(m1 == 0 && e1 == 0))
    {
        convertPart(e1, m1, real_e1);
    }
    convertPart(e2, m2, real_e2);
    convertPart(e3, m3, real_e3);
}

void FMA::convertPart(uint32_t ein, uint32_t &fin, int32_t &eout)
{
    eout = int32_t(ein) - bias;
    fin = (1ULL << mantissa_bits) | fin;
}

bool FMA ::multiplyExact()
{
    sign_product = s2 ^ s3;
    exponent_product = real_e2 + real_e3;
    mantissa_product = static_cast<uint64_t>(m2) * static_cast<uint64_t>(m3);
    std::bitset<64> b(mantissa_product);
    // norm tells us if the result we got is actually two digits before the comme (we can have either 1 digit or two before comma)
    bool norm = mantissa_product & (uint64_t(1) << (2 * mantissa_bits + 1));
    if (norm)
    {
        // so that always last 1 in mantissa_product represents the leading 1
        exponent_product++;
    }
    // check for overflow
    if ((exponent_product + bias) >= (int32_t(1 << exponent_bits) - 1))
    {
        write_overflow(sign_product);
        return true;
    }
    // no need for underflow check

    return false;
}

void FMA ::alignAddend()
{
    // top will from now on always represent the index of the 1. of the result
    top = 63 - __builtin_clzll(mantissa_product);
    // if adder is 0 , leave it like that
    if (m1 != 0 || e1 != 0)
    {
        // those are lower bits that we will cut from result later
        int shift = top - int(mantissa_bits);
        // exponent difference
        int32_t delta = exponent_product - real_e1;
        bigM1 = m1;
        // bigM1 is the alligned m1
        bigM1 = bigM1 << shift;
    }
}
void FMA ::main_funct()
{
    // this is the 64 but container of the mul + add
    uint64_t result_mantissa;
    // needed for rounding
    bool guard = 0;
    bool sticky = 0;
    // if adder not 0
    if (m1 != 0 || e1 != 0)
    {
        final_exp = std::max(real_e1, exponent_product);
        int32_t delta = exponent_product - real_e1;
        // those are useful for different sign addition , since for same sign addition it doesn't matter which one takes what variable
        uint64_t mantissa_to_shift;
        uint64_t other_mantissa;
        if (delta < 0)
        {
            // sign determination , since we will always be subbing other mantissa from mantissa_to_shift
            if (sign_product != s1)
            {
                if (s1 == 0)
                {
                    final_sign = 0;
                }
                else
                {
                    final_sign = 1;
                }
            }
            else
            {
                final_sign = s1;
            }
            mantissa_to_shift = mantissa_product;
            other_mantissa = bigM1;
        }
        else
        {
            if (sign_product != s1)
            {
                if (sign_product == 0)
                {
                    final_sign = 0;
                }
                else
                {
                    final_sign = 1;
                }
            }
            else
            {
                final_sign = s1;
            }

            mantissa_to_shift = bigM1;
            other_mantissa = mantissa_product;
        }
        // if difference is 0 , then we should assure that mantissa_shift holds the smallest mantissa
        if (delta == 0)
        {
            // if mul part and adder have same sign , it is not important which one is bigger cuz we add them
            // but if not same sign , we ensure that mantissa_toShift has smaller mantissa , cuz we do other - to_shift
            if (s1 != sign_product)
            {
                if (mantissa_to_shift > other_mantissa)
                {
                    std::swap(mantissa_to_shift, other_mantissa);
                    final_sign = s1 == 0 ? 0 : 1;
                }
            }
        }
        // determining G/S for the mantissa we're shifting
        for (int i = 0; i < std::abs(delta); i++)
        {
            sticky = sticky | guard;
            guard = mantissa_to_shift & 1;
            mantissa_to_shift = mantissa_to_shift >> 1;
        }

        if (sign_product == s1)
        {
            result_mantissa = mantissa_to_shift + other_mantissa;
            // also if result is two digits after comma , we normalize
            if (result_mantissa & (uint64_t(1) << (top + 1)))
            {
                final_exp++;
                // updating top so that it always points to the 1.
                top++;
                // check for overflow since final_exp is increased
                if (final_exp + bias >= int32_t((1u << exponent_bits) - 1u))
                {

                    write_overflow(final_sign);
                    return;
                }
            }
        }
        else
        {
            // if delta is not null , then mantissa_to_shift is already shifted , meaning it's smaller now
            result_mantissa = other_mantissa - mantissa_to_shift;
            // early check for result =0
            if (result_mantissa == 0)
            {
                ro.write(0);
                zero.write(true);
                sign.write(0);
                return;
            }
            // not yet renormalizing until we round so that grs' logic stays consistent
        }
        if (sign_product != s1)
        {
            // if signs are different of product and adder , then grs actually represent grs of shifted manitssa
            // with this logic , grs represents again natural bits
            if (!(guard == 0 && sticky == 0))
            {
                result_mantissa -= 1;
                if (guard == 0 && sticky == 1)
                {
                    guard = 1;
                }
                else if (guard == 1 && sticky == 1)
                {
                    guard = 0;
                }
            }
        }
    }
    else
    {
        // if adder is 0 , then exponent of the multiplier is decisive for underflow and should be checked
        if (exponent_product + bias < 0)
        {
            write_underflow(sign_product);
            return;
        }
        result_mantissa = mantissa_product;
        final_sign = sign_product;
        final_exp = exponent_product;
    }
    //  update grs , slow propagate them from left to right becuz of bits that will be cut
    int loopSize = top - int(mantissa_bits);
    for (int i = 0; i < loopSize; i++)
    {
        sticky = sticky | guard;
        guard = result_mantissa & 1;
        result_mantissa = result_mantissa >> 1;
        // bucez mantissa is shifted , top is decreased
        top--;
    }
    uint32_t biased_exp = final_exp + bias;
    // this the mantissa with leading 1. and having mantissa_bits after it
    final_mantissa = ((((1ULL << (mantissa_bits + 1)) - 1) << (top - mantissa_bits)) & result_mantissa) >> (top - mantissa_bits);
    // if round mantissa is again true , means under/overflow cases are already handled
    if (!round_mantissa(final_mantissa, biased_exp, final_sign, guard, sticky))
    {
        // now after rounding we renormalize our result for different sign adding
        if (s1 != sign_product)
        {
            //  first check if mantissa has become 0 after rounding
            if (final_mantissa == 0)
            {
                write_underflow(final_sign);
                return;
            }
            while (final_mantissa >> mantissa_bits != 1)
            {
                final_mantissa = final_mantissa << 1;
                // means underflow cuz can't subtract further
                if (biased_exp == 0)
                {
                    write_underflow(final_sign);
                    return;
                }
                biased_exp--;
            }
        }
        final_mantissa &= (~(1u << mantissa_bits));
        ro.write((final_sign << 31) | (biased_exp << mantissa_bits) | (final_mantissa));
        sign.write(final_sign);
    }
}
bool FMA ::round_mantissa(uint32_t &mantissa, uint32_t &exp, uint32_t &par_sign, bool guard, bool sticky)
{
    // check if rounding is even required
    if (guard == 0 && sticky == 0)
    {
        return false;
    }
    inexact.write(true);
    switch (round_mode)
    {
    case 0:
    {

        bool is_tie = (guard == 1) && (sticky == 0);
        bool lsb_is_odd = (mantissa & 1) == 1;
        bool more_than_half = (guard == 1) && (sticky == 1);
        // rounding only happens when those conditions are true
        if ((is_tie && lsb_is_odd) || more_than_half)
        {

            if (increase_mantissa_by_one(mantissa, exp, par_sign))
            {
                return true;
            }
        }

        break;
    }
    case 1:
    {

        bool is_tie = (guard == 1) && (sticky == 0);
        bool more_than_half = (guard == 1) && (sticky == 1);

        if (is_tie || more_than_half)
        {

            if (increase_mantissa_by_one(mantissa, exp, par_sign))
            {
                return true;
            }
        }

        break;
    }
    case 2:
    {

        break;
    }
    case 3:
    {
        if (par_sign == 0)
        {
            if (increase_mantissa_by_one(mantissa, exp, par_sign))
            {
                return true;
            }
        }

        break;
    }
    case 4:
    {
        if (par_sign == 1)
        {
            if (increase_mantissa_by_one(mantissa, exp, par_sign))
            {
                return true;
            }
        }

        break;
    }
    }

    return false;
}
// increases mantissa by one , true when overflow is already handled
bool FMA ::increase_mantissa_by_one(uint32_t &mantissa, uint32_t &exp, uint32_t par_sign)
{
    if (mantissa == ((1u << (mantissa_bits + 1)) - 1))
    {
        mantissa = (1 << mantissa_bits);
        exp++;

        if (exp == ((1u << exponent_bits) - 1u))
        {
            write_overflow(par_sign);
            return true;
        }
    }
    else
    {
        mantissa += 1;
    }
    return false;
};
void FMA::write_overflow(uint32_t par_sign)
{
    overflow.write(true);
    inexact.write(true);
    // inf based on sign
    if (par_sign == 1)
    {
        ro.write(negative_inf_constant);
        sign.write(1);
    }
    else
    {
        ro.write(positive_inf_constant);
        sign.write(0);
    }
}
void FMA::write_underflow(uint32_t par_sign)
{
    // in underflow sign is important to know if real result is pos or neg
    if (par_sign == 0)
    {
        ro.write(0);
    }
    else
    {
        ro.write(0x80000000);
    }
    underflow.write(true);
    inexact.write(true);
    zero.write(true);
    sign.write(par_sign);
}
bool FMA::is_nan(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa)
{
    if (exponent_value == ((1u << exponent_bits) - 1u))
    {
        return mantissa != 0;
    }
    return false;
}

bool FMA::is_inf(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa)
{
    if (exponent_value == (((1u << exponent_bits) - 1u)))
    {
        return mantissa == 0;
    }
    return false;
}