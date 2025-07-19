#include <systemc>
#include <cstdint>

using namespace sc_core;
using namespace sc_dt;

SC_MODULE(AddSub)
{
    sc_in<uint32_t> r1;
    sc_in<uint32_t> r2;
    sc_in<sc_bv<4>> op;
    sc_out<uint32_t> ro;
    sc_out<bool> overflow;
    sc_out<bool> underflow;
    sc_out<bool> inexact;
    sc_out<bool> nan;
    sc_out<bool> sign;
    sc_out<bool> zero;
    uint32_t rounding_option;
    uint32_t mantissa_bits;
    uint32_t exponent_bits;
    uint32_t exponent_r1;
    uint32_t mantissa_r1;
    uint32_t exponent_r2;
    uint32_t mantissa_r2;
    uint32_t sign_r1;
    uint32_t sign_r2;
    uint32_t nan_constant;
    uint32_t positive_inf_constant;
    uint32_t negative_inf_constant;

    SC_HAS_PROCESS(AddSub);
    AddSub(sc_core::sc_module_name name,
           uint32_t exp_bits,
           uint32_t man_bits, uint32_t rounding_option);
    void exec();
    void extract();
    void same_sign_add();
    void cont_sign_add();
    bool is_nan(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa);
    bool is_inf(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa);
    bool round_mantissa(bool add_same_sign, uint32_t &mantissa, uint32_t &exp, uint32_t &sign, bool guard, bool sticky);
    bool increase_mantissa_by_one(uint32_t &mantissa, uint32_t &exp, uint32_t sign);
    bool decrease_mantissa_by_one(uint32_t &mantissa, uint32_t &exp);
};