#include "../include/Min.hpp"
#include <systemc>
#include <cstdint>
using namespace sc_core;
using namespace sc_dt;

 // same logic as max except for one line who is reversed 
Min::Min(sc_module_name name, uint32_t exp_bits, uint32_t man_bits)
    : sc_module(name), exponent_bits(exp_bits), mantissa_bits(man_bits) {
    SC_METHOD(exec);
	sensitive<< clk.pos();
}

void Min::exec() {
    nan.write(false);
    overflow.write(false);
    underflow.write(false);
    inexact.write(false);

    extract();
    evaluate();
}

void Min::extract() {
    val1 = r1.read();
    val2 = r2.read();
    sign1 = val1 >> 31;
    sign2 = val2 >> 31;
}

void Min::evaluate() {
    if (is_nan(val1) && is_nan(val2)) {
        nan.write(true);
        ro.write(get_nan());
        return;
    }
    if (is_nan(val1)) {
        ro.write(val2);
        return;
    }
    if (is_nan(val2)) {
        ro.write(val1);
        return;
    }

    if (is_inf(val1) && is_inf(val2)) {
        if (val1 == val2) {
            ro.write(val1);
        } else {
            nan.write(true);
            ro.write(get_nan());
        }
        return;
    }
    if (is_inf(val1)) {
        ro.write(val2);
        return;
    }
    if (is_inf(val2)) {
        ro.write(val1);
        return;
    }

    if (val1 == 0 && val2 == 0) {
        ro.write(0);
        return;
    }

    uint32_t result = (compare(val1, val2) <= 0) ? val1 : val2;
    ro.write(result);
    sign.write((result >> 31) & 1);
    uint32_t exp_mask = ((1u << exponent_bits) - 1) << mantissa_bits;
    uint32_t man_mask = (1u << mantissa_bits) - 1;
    bool is_zero = ((result & exp_mask) == 0) && ((result & man_mask) == 0);
    zero.write(is_zero);
}

bool Min::is_nan(uint32_t val) const {
    uint32_t exp = (val >> mantissa_bits) & ((1u << exponent_bits) - 1);
    uint32_t mant = val & ((1u << mantissa_bits) - 1);
    return (exp == ((1u << exponent_bits) - 1)) && (mant != 0);
}

bool Min::is_inf(uint32_t val) const {
    uint32_t exp = (val >> mantissa_bits) & ((1u << exponent_bits) - 1);
    uint32_t mant = val & ((1u << mantissa_bits) - 1);
    return (exp == ((1u << exponent_bits) - 1)) && (mant == 0);
}

uint32_t Min::get_nan() const {
    return (((1u << exponent_bits) - 1) << mantissa_bits) | 0x1;
}

int Min::compare(uint32_t a, uint32_t b) const {
    bool sign_a = a >> 31;
    bool sign_b = b >> 31;
    if (sign_a != sign_b)
        return sign_a ? 1 : -1;
    if (a == b) return 0;
    return (sign_a ? a > b : a < b) ? -1 : 1;
}
