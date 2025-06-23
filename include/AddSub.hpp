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
    uint32_t mantissa_bits;
    uint32_t exponent_bits;
    uint32_t exponent_r1;
    uint32_t mantissa_r1;
    uint32_t exponent_r2;
    uint32_t mantissa_r2;
    uint32_t sign_r1;
    uint32_t sign_r2;

    SC_HAS_PROCESS(AddSub);
    AddSub(sc_core::sc_module_name name,
           uint32_t exp_bits,
           uint32_t man_bits);
    void exec();
    void extract();
    void add();
    bool is_nan(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa);
    bool is_inf(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa);
};