#include "../include/AddSub.hpp"
#include <systemc>
#include <cstdint>
using namespace sc_core;
using namespace sc_dt;

AddSub::AddSub(sc_module_name name, uint32_t e_bits, uint32_t m_bits, uint32_t roption)
    : sc_module(name), exponent_bits(e_bits), mantissa_bits(m_bits), rounding_option(roption)
{
    SC_METHOD(exec);
    sensitive << r1 << r2;
}

void AddSub::exec()
{
    overflow.write(false);
    underflow.write(false);
    inexact.write(false);
    nan.write(false);
    extract();
    nan_constant = (((1 << exponent_bits) - 1) << mantissa_bits) + 1;
    positive_inf_constant = ((1 << exponent_bits) - 1) << mantissa_bits;
    negative_inf_constant = (1 << 31) | (positive_inf_constant);
    // case any of operands in nan return the nan constant
    if (is_nan(exponent_r1, exponent_bits, mantissa_r1) || is_nan(exponent_r2, exponent_bits, mantissa_r2))
    {
        nan.write(true);
        ro.write(nan_constant);
        return;
    }
    // both operands are inf
    if (is_inf(exponent_r1, exponent_bits, mantissa_r1) && is_inf(exponent_r2, exponent_bits, mantissa_r2))
    {
        if (sign_r1 == sign_r2)
        {
            ro.write(r1.read());
        }
        else
        {
            nan.write(true);
            ro.write(nan_constant);
        }
        return;
    }
    // one operand is inf , other is normal , then return the inf as it is
    if (is_inf(exponent_r1, exponent_bits, mantissa_r1))
    {
        ro.write(r1.read());
        return;
    }
    if (is_inf(exponent_r2, exponent_bits, mantissa_r2))
    {
        ro.write(r2.read());
        return;
    }
    // now both operands are normal
    // if one of operands is 0 return other operand
    if (r1.read() == 0 || (r1.read() == (1 << 31)))
    {
        ro.write(r2.read());
        return;
    }
    else if (r2.read() == 0 || (r2.read() == (1 << 31)))
    {
        ro.write(r1.read());
        return;
    }
    if ((op.read().to_uint() == 8 && sign_r1 == sign_r2) || (op.read().to_uint() == 9 && sign_r1 != sign_r2))
    {
        // if we are perfomring subtraction , adjust signs to do add equivalently
        if (op.read().to_uint() == 9)
        {
            if (sign_r1 == 0)
            {
                sign_r2 = 0;
            }
            else
            {
                sign_r2 = 1;
            }
        }

        same_sign_add();
    }
    else if ((op.read().to_uint() == 8 && sign_r1 != sign_r2) || (op.read().to_uint() == 9 && sign_r1 == sign_r2))
    {
        // if we are perfomring subtraction , adjust signs to do add equivalently
        if (op.read().to_uint() == 9)
        {
            if (sign_r1 == 0)
            {
                sign_r2 = 1;
            }
            else
            {
                sign_r2 = 0;
            }
        }
        cont_sign_add();
    }
}
void AddSub::extract()
{
    sign_r1 = r1.read() >> 31;
    sign_r2 = r2.read() >> 31;

    exponent_r1 = (exponent_bits == 0) ? 0 : (r1.read() << 1) >> (mantissa_bits + 1);
    exponent_r2 = (exponent_bits == 0) ? 0 : (r2.read() << 1) >> (mantissa_bits + 1);

    mantissa_r1 = (mantissa_bits == 0) ? 0 : (r1.read() << (32 - mantissa_bits)) >> (32 - mantissa_bits);
    mantissa_r2 = (mantissa_bits == 0) ? 0 : (r2.read() << (32 - mantissa_bits)) >> (32 - mantissa_bits);
}
void AddSub::same_sign_add()
{
    int32_t diff = (int32_t)exponent_r1 - (int32_t)exponent_r2;
    uint32_t added_mantissas;
    uint32_t res_exp;
    bool guard = 0;
    bool sticky = 0;
    // if of same sign
    // add leading 1.
    mantissa_r1 = (1 << mantissa_bits) | mantissa_r1;
    mantissa_r2 = (1 << mantissa_bits) | mantissa_r2;
    res_exp = diff > 0 ? exponent_r1 : exponent_r2;

    // shifting mantissa with smaller exponent and add to unshifted
    uint32_t shifted_mantissa = diff >= 0 ? mantissa_r2 : mantissa_r1;
    for (int i = 0; i < std::abs(diff); i++)
    {
        bool lost_bit = shifted_mantissa & 1;
        sticky = (sticky) | (guard);
        guard = lost_bit;
        shifted_mantissa = shifted_mantissa >> 1;
    }

    added_mantissas = shifted_mantissa + ((diff >= 0) ? mantissa_r1 : mantissa_r2);

    // check if last digit of added mantissa is one , which means we have two digits after coma
    bool must_shift = (added_mantissas >> (mantissa_bits + 1));
    // if there are then shift once
    if (must_shift)
    {
        bool lost_bit = added_mantissas & 1;
        sticky = (sticky) | (guard);
        guard = lost_bit;
        added_mantissas = added_mantissas >> 1;
        res_exp++;
        // after incrementing res exp ,check if it becomes full of 1 --> should return inf
        if (res_exp == ((exponent_bits == 32)
                            ? std::numeric_limits<uint32_t>::max()
                            : ((1u << exponent_bits) - 1u)))
        {
            overflow.write(true);
            inexact.write(true);
            ro.write(sign_r1 == 0 ? positive_inf_constant : negative_inf_constant);
            return;
        }
    }
    if (!round_mantissa(true, added_mantissas, res_exp, sign_r1, guard, sticky))
    {
        ro.write((sign_r1 << 31) | (res_exp << mantissa_bits) | added_mantissas & (~(1u << mantissa_bits)));
    }
}
void AddSub::cont_sign_add()
{
    int32_t diff = (int32_t)exponent_r1 - (int32_t)exponent_r2;
    uint32_t res_exp;
    uint32_t subbed_mantissas;
    uint32_t bigger_mantissa;
    bool guard = 0;
    bool sticky = 0;
    res_exp = diff > 0 ? exponent_r1 : exponent_r2;
    mantissa_r1 = (1 << mantissa_bits) | mantissa_r1;
    mantissa_r2 = (1 << mantissa_bits) | mantissa_r2;
    uint32_t mantissa_to_shift;
    uint32_t res_sign;
    // determin bigger for sign determination
    if (diff < 0 || (diff == 0 && mantissa_r1 < mantissa_r2))
    {
        bigger_mantissa = mantissa_r2;
        mantissa_to_shift = mantissa_r1;
        if (sign_r1 == 0)
        {
            res_sign = 1;
        }
        else
        {
            res_sign = 0;
        }
    }
    else
    {
        bigger_mantissa = mantissa_r1;
        mantissa_to_shift = mantissa_r2;
        if (sign_r1 == 0)
        {
            res_sign = 0;
        }
        else
        {
            res_sign = 1;
        }
    }
    for (int i = 0; i < std::abs(diff); i++)
    {
        bool lost_bit = mantissa_to_shift & 1;
        sticky = (sticky) | (guard);
        guard = lost_bit;
        mantissa_to_shift = mantissa_to_shift >> 1;
    }

    // final mantissa
    subbed_mantissas = bigger_mantissa - mantissa_to_shift;
    if (subbed_mantissas == 0)
    {
        ro.write(0);
        return;
    }
    if (!round_mantissa(false, subbed_mantissas, res_exp, res_sign, guard, sticky))
    {
        // bring mantissa to scientific notation , since it can be 0. after substraction
        while (subbed_mantissas >> mantissa_bits != 1)
        {
            subbed_mantissas = subbed_mantissas << 1;
            // means underflow cuz can't subtract further
            if (res_exp == 0)
            {
                underflow.write(true);
                inexact.write(true);
                ro.write(0);
                return;
            }
            res_exp--;
        }

        // eliminate 1. leading
        subbed_mantissas &= (~(1u << mantissa_bits));
        ro.write((res_sign << 31) | (res_exp << mantissa_bits) | (subbed_mantissas));
    }
}

bool AddSub::is_nan(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa)
{
    if (exponent_value == ((exponent_bits == 32)
                               ? std::numeric_limits<uint32_t>::max()
                               : ((1u << exponent_bits) - 1u)))
    {
        return mantissa != 0;
    }
    return false;
}

bool AddSub::is_inf(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa)
{
    if (exponent_value == ((exponent_bits == 32)
                               ? std::numeric_limits<uint32_t>::max()
                               : ((1u << exponent_bits) - 1u)))
    {
        return mantissa == 0;
    }
    return false;
}
// add is a paramter , when true , means add same sign , when false , cont signs
// this function rounds mantissa (preserving leading 1.) and changes exp accordingly . returns true when overflow cases already handled

bool AddSub::round_mantissa(bool add_same_sign, uint32_t &mantissa, uint32_t &exp, uint32_t &sign, bool guard, bool sticky)
{
    if (guard == 0 && sticky == 0)
    {
        return false;
    }
    inexact.write(true);
    switch (rounding_option)
    {
    case 0:
    {

        bool is_tie = (guard == 1) && (sticky == 0);
        bool lsb_is_odd = (mantissa & 1) == 1;
        bool more_than_half = (guard == 1) && (sticky == 1);
        // rounding only happens when those conditions are true
        if ((is_tie && lsb_is_odd) || more_than_half)
        {

            if (add_same_sign)
            {
                if (increase_mantissa_by_one(mantissa, exp, sign))
                {
                    ro.write(sign == 0 ? positive_inf_constant : negative_inf_constant);
                    return true;
                }
            }
            else
            {
                if (decrease_mantissa_by_one(mantissa, exp))
                {
                    ro.write(0);
                    return true;
                }
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

            if (add_same_sign)
            {
                if (increase_mantissa_by_one(mantissa, exp, sign))
                {
                    ro.write(sign == 0 ? positive_inf_constant : negative_inf_constant);
                    return true;
                }
            }
            else if (more_than_half)
            {
                if (decrease_mantissa_by_one(mantissa, exp))
                {
                    ro.write(0);
                    return true;
                }
            }
        }

        break;
    }
    case 2:
    {
        if (!add_same_sign)
        {
            if (decrease_mantissa_by_one(mantissa, exp))
            {
                ro.write(0);
                return true;
            }
        }
        break;
    }
    case 3:
    {
        if (add_same_sign && sign == 0)
        {
            if (increase_mantissa_by_one(mantissa, exp, sign))
            {
                ro.write(positive_inf_constant);
                return true;
            }
        }
        else if (!add_same_sign && sign == 1)
        {
            if (decrease_mantissa_by_one(mantissa, exp))
            {
                ro.write(0);
                return true;
            }
        }
        break;
    }
    case 4:
    {
        if (add_same_sign && sign == 1)
        {
            if (increase_mantissa_by_one(mantissa, exp, sign))
            {
                ro.write(negative_inf_constant);
                return true;
            }
        }
        else if (!add_same_sign && sign == 0)
        {
            if (decrease_mantissa_by_one(mantissa, exp))
            {
                ro.write(0);
                return true;
            }
        }
        break;
    }
    }

    return false;
}
// increases mantissa by one , true when overflow is already handled
bool AddSub::increase_mantissa_by_one(uint32_t &mantissa, uint32_t &exp, uint32_t sign)
{
    if (mantissa == ((1u << (mantissa_bits + 1)) - 1))
    {
        mantissa = (1 << mantissa_bits);
        exp++;

        if (exp == ((exponent_bits == 32)
                        ? std::numeric_limits<uint32_t>::max()
                        : ((1u << exponent_bits) - 1u)))
        {
            overflow.write(true);
            inexact.write(true);
            return true;
        }
    }
    else
    {
        mantissa += 1;
    }
    return false;
}
// deccreases mantissa by one , true when result already handled
bool AddSub::decrease_mantissa_by_one(uint32_t &mantissa, uint32_t &exp)
{
    mantissa--;
    // this tells caller program to write(0) , this should not represent an underflow case
    if (mantissa == 0)
    {
        return true;
    }
    if (mantissa >> mantissa_bits != 1)
    {
        if (exp == 0)
        {
            underflow.write(true);
            inexact.write(true);
            return true;
        }
    }
    mantissa <<= 1;
    exp--;
    return false;
}