#include <systemc>
#include <cstdint>
using namespace sc_core;
using namespace sc_dt;
SC_MODULE(Min) {
    sc_in<uint32_t> r1;
    sc_in<uint32_t> r2;
    sc_out<uint32_t> ro;
    sc_out<bool> nan;
    sc_out<bool> overflow;
    sc_out<bool> underflow;
    sc_out<bool> inexact;

    uint32_t mantissa_bits;
    uint32_t exponent_bits;
    uint32_t val1, val2;
    uint32_t sign1, sign2;

    SC_HAS_PROCESS(Min);
    Min(sc_module_name name, uint32_t exp_bits, uint32_t man_bits);

    void exec();
    void extract();
    void evaluate();
    bool is_nan(uint32_t val) const;
    bool is_inf(uint32_t val) const;
    uint32_t get_nan() const;
    int compare(uint32_t a, uint32_t b) const;
}; // logic of max the same as min 