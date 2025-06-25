#include "../include/Adder.hpp"
#include <systemc>
#include <cstdint>
using namespace sc_core;
using namespace sc_dt;

Adder::Adder(sc_module_name name, uint32_t e_bits, uint32_t m_bits)
    : sc_module(name), exponent_bits(e_bits), mantissa_bits(m_bits)
{
    SC_METHOD(exec);
    sensitive << r1 << r2;
}

void Adder::exec()
{
    overflow.write(false);
    underflow.write(false);
    inexact.write(false);
    nan.write(false);
    extract();
    add();
}
void Adder::extract()
{
    sign_r1 = r1.read() >> 31;
    sign_r2 = r2.read() >> 31;

    exponent_r1 = (exponent_bits == 0) ? 0 : (r1.read() << 1) >> (mantissa_bits + 1);
    exponent_r2 = (exponent_bits == 0) ? 0 : (r2.read() << 1) >> (mantissa_bits + 1);

    mantissa_r1 = (mantissa_bits == 0) ? 0 : (r1.read() << (32 - mantissa_bits)) >> (32 - mantissa_bits);
    mantissa_r2 = (mantissa_bits == 0) ? 0 : (r2.read() << (32 - mantissa_bits)) >> (32 - mantissa_bits);
}
void Adder::add()
{
    uint32_t nan_constant = (((1 << exponent_bits) - 1) << mantissa_bits) + 1;
    uint32_t positive_inf_constant = ((1 << exponent_bits) - 1) << mantissa_bits;
    uint32_t negative_inf_constant = (1 << 31) | (positive_inf_constant);
    int32_t diff = exponent_r1 - exponent_r2;
    uint32_t res;
    uint32_t added_mantissas;
    uint32_t res_exp;
    uint32_t subbed_mantissas;
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
            ro.write(nan_constant);
        }
        return;
    }
    // one operand is inf , other is normal , then return the inf as it is
    if (is_inf(exponent_r1, exponent_bits, mantissa_r1))
    {
        ro.write(r1.read());
    }
    if (is_inf(exponent_r2, exponent_bits, mantissa_r2))
    {
        ro.write(r2.read());
    }
    // now both operands are normal
    // if one of operands is 0 , or other operand is considered too big , return other operand
    if (r1.read() == 0 || (r1.read() == (1 << 31)) || (diff <= -32))
    {
        if (diff <= -32)
            inexact.write(true);
        ro.write(r2.read());
        return;
    }
    else if (r2.read() == 0 || (r2.read() == (1 << 31)) || diff >= 32)
    {
        if (diff >= 32)
        {
            inexact.write(true);
        }
        ro.write(r1.read());
        return;
    }
    // if of same sign
    // add leading 1.
    mantissa_r1 = (1 << mantissa_bits) | mantissa_r1;
    mantissa_r2 = (1 << mantissa_bits) | mantissa_r2;
    if (sign_r1 == sign_r2)
    {

        if (diff != 0)
        {
            res_exp = diff > 0 ? exponent_r1 : exponent_r2;
            // checking if first |diff| bits of the mantissa to shift  is 0 or not , to know if bits wil be lost or not when shifting
            if (((diff > 0 ? mantissa_r2 : mantissa_r1) << (32 - std::abs(diff)) >> (32 - std::abs(diff))) != 0)
            {
                inexact.write(true);
            }
            // shifting mantissa with smaller exponent and add to unshifted
            uint32_t shifted_mantissa = (diff > 0 ? mantissa_r2 : mantissa_r1) >> std::abs(diff);

            added_mantissas = shifted_mantissa + ((diff > 0) ? mantissa_r1 : mantissa_r2);

            // check if last digit of added mantissa is one , which means we have two digits after coma
            bool must_shift = (added_mantissas >> (mantissa_bits + 1));
            // if there are then shift once
            if (must_shift)
            {
                added_mantissas = added_mantissas >> 1;
                res_exp++;
                // after incrementing res exp ,check if it becomes full of 1 --> should return inf
                if (res_exp == ((exponent_bits == 32)
                                    ? std::numeric_limits<uint32_t>::max()
                                    : ((1u << exponent_bits) - 1u)))
                {
                    overflow.write(true);
                    ro.write(sign_r1 == 0 ? positive_inf_constant : negative_inf_constant);
                    return;
                }
            }
        }
        else
        {
            res_exp = exponent_r1;
            // add directly and shift once since result is always two digits after comma
            added_mantissas = (mantissa_r1 + mantissa_r2) >> 1;
            res_exp++;
            // after incrementing res exp ,check if it becomes full of 1 --> should return inf
            if (res_exp == ((exponent_bits == 32)
                                ? std::numeric_limits<uint32_t>::max()
                                : ((1u << exponent_bits) - 1u)))
            {
                overflow.write(true);
                ro.write(sign_r1 == 0 ? positive_inf_constant : negative_inf_constant);
                return;
            }
        }
        // eliminate leading 1.
        added_mantissas &= (~(1u << mantissa_bits));

        ro.write((sign_r1 << 31) | (res_exp << mantissa_bits) | (added_mantissas));
    }
    else
    {

        uint32_t bigger_mantissa;
        uint32_t smaller_mantissa;

        if (diff != 0)
        {

            res_exp = diff > 0 ? exponent_r1 : exponent_r2;
            // check if first |diff| bits of mantissa is null or not
            if (((diff > 0 ? mantissa_r2 : mantissa_r1) << (32 - std::abs(diff)) >> (32 - std::abs(diff))) != 0)
            {
                inexact.write(true);
            }
            // shifting mantissa with smaller exponent and add to unshifted
            uint32_t shifted_mantissa = (diff > 0 ? mantissa_r2 : mantissa_r1) >> std::abs(diff);
            // assigning big and small mantissa , needed for later when determening the sign
            bigger_mantissa = (diff > 0 ? mantissa_r1 : mantissa_r2);
            smaller_mantissa = shifted_mantissa;
            // final mantissa
            subbed_mantissas = (diff > 0 ? mantissa_r1 : mantissa_r2) - shifted_mantissa;
        }
        else
        {
            res_exp = exponent_r1;
            // substract bigger from smaller mantissa to avoid overlapping values with substraction in unsigned
            bigger_mantissa = (mantissa_r1 < mantissa_r2 ? mantissa_r2 : mantissa_r1);
            smaller_mantissa = (mantissa_r1 < mantissa_r2 ? mantissa_r1 : mantissa_r2);
            subbed_mantissas = bigger_mantissa - smaller_mantissa;
            // special check :ensure that 0 is result when substracting two equals numbers
            if (subbed_mantissas == 0)
            {
                ro.write(0);
                return;
            }
        }
        // bring mantissa to scientific notation , since it can be 0. after substraction
        while (subbed_mantissas >> mantissa_bits != 1)
        {
            subbed_mantissas = subbed_mantissas << 1;
            // means underflow cuz can't subtract further
            if (res_exp == 0)
            {
                underflow.write(true);
                ro.write(0);
                return;
            }
            res_exp--;
        }
        // determine sign of result
        uint32_t res_sign;
        if (bigger_mantissa == mantissa_r1 && sign_r1 == 0)
        {
            res_sign = 0;
        }
        else if (bigger_mantissa == mantissa_r1 && sign_r1 == 1)
        {
            res_sign = 1;
        }
        else if (bigger_mantissa == mantissa_r2 && sign_r1 == 0)
        {
            res_sign = 1;
        }
        else
        {
            res_sign = 0;
        }
        // eliminate 1. leading
        subbed_mantissas &= (~(1u << mantissa_bits));
        ro.write((res_sign << 31) | (res_exp << mantissa_bits) | (subbed_mantissas));
    }
}
bool Adder::is_nan(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa)
{
    if (exponent_value == ((exponent_bits == 32)
                               ? std::numeric_limits<uint32_t>::max()
                               : ((1u << exponent_bits) - 1u)))
    {
        return mantissa != 0;
    }
    return false;
}
bool Adder::is_inf(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa)
{
    if (exponent_value == ((exponent_bits == 32)
                               ? std::numeric_limits<uint32_t>::max()
                               : ((1u << exponent_bits) - 1u)))
    {
        return mantissa == 0;
    }
    return false;
}
