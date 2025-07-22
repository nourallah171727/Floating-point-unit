#include "../include/Max.hpp"

Max::Max(sc_module_name name, uint32_t exp_bits, uint32_t man_bits)
    : sc_module(name), exponent_bits(exp_bits), mantissa_bits(man_bits) {
    SC_METHOD(exec);
	sensitive<< clk.pos();
}

void Max::exec() {
    nan.write(false);
    overflow.write(false);
    underflow.write(false);
    inexact.write(false);

    extract();
    evaluate();
}

void Max::extract() {
    val1 = r1.read();
    val2 = r2.read(); // read inputs
    sign1 = val1 >> 31;
    sign2 = val2 >> 31; // extract signs 
}

void Max::evaluate() {
    if (is_nan(val1) && is_nan(val2)) {
        nan.write(true);
        ro.write(get_nan());
        return; // both are nan 
    }
    if (is_nan(val1)) {
        ro.write(val2);
        return;
    }
    if (is_nan(val2)) {
        ro.write(val1);
        return; // one is nan 
    }

    if (is_inf(val1) && is_inf(val2)) {
        if (val1 == val2) {
            ro.write(val1); // both inf and equal 
        } else {
            nan.write(true);
            ro.write(get_nan()); //both inf and not equal 
        }
        return;
    }
    if (is_inf(val1)) {
        ro.write(val1);
        return; 
    }
    if (is_inf(val2)) {
        ro.write(val2);
        return; // only one is inf 
    }

    if (val1 == 0 && val2 == 0) {
        ro.write(0);
        return; // both are 0   
         }

    uint32_t result = (compare(val1, val2) >= 0) ? val1 : val2;
    ro.write(result); //compare call 
    sign.write((result >> 31) & 1);
    uint32_t exp_mask = ((1u << exponent_bits) - 1) << mantissa_bits;
    uint32_t man_mask = (1u << mantissa_bits) - 1;
    bool is_zero = ((result & exp_mask) == 0) && ((result & man_mask) == 0);
    zero.write(is_zero); //setting flags 
}

bool Max::is_nan(uint32_t val) const {
    uint32_t exp = (val >> mantissa_bits) & ((1u << exponent_bits) - 1);
    uint32_t mant = val & ((1u << mantissa_bits) - 1);
    return (exp == ((1u << exponent_bits) - 1)) && (mant != 0); // check if nan 
}

bool Max::is_inf(uint32_t val) const {
    uint32_t exp = (val >> mantissa_bits) & ((1u << exponent_bits) - 1);
    uint32_t mant = val & ((1u << mantissa_bits) - 1);
    return (exp == ((1u << exponent_bits) - 1)) && (mant == 0); // check if inf 
}

uint32_t Max::get_nan() const {
    return (((1u << exponent_bits) - 1) << mantissa_bits) | 0x1; // nan 
}

int Max::compare(uint32_t a, uint32_t b) const {
    bool sign_a = a >> 31;
    bool sign_b = b >> 31;
    if (sign_a != sign_b)
        return sign_a ? 1 : -1;
    if (a == b) return 0;
    return (sign_a ? a > b : a < b) ? -1 : 1; 
    // if sign are different just compare signs, if not then it depends on the sign 
    //if positive then take the bigger value after the sign and 
    //if negative take the smaller value after the sign 
}