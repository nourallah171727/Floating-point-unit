#include <systemc>
#include <cstdint>

using namespace sc_core;

SC_MODULE(FMA)
{
    uint32_t positive_inf_constant;
    uint32_t negative_inf_constant;

	sc_in<bool> clk;
	sc_in<uint32_t> r1, r2, r3;
    sc_out<uint32_t> ro;
    sc_out<bool> zero, sign, overflow, underflow, inexact, nan;

    const uint32_t mantissa_bits;
    const uint32_t exponent_bits;
    const int32_t bias;
    const int round_mode;
    int32_t real_e1, real_e2, real_e3;
    bool s1, s2, s3;
    uint32_t e1, e2, e3;
    uint32_t m1, m2, m3;
    uint64_t bigM1;

    bool sign_product;
    int32_t exponent_product;
    int32_t top;

    uint64_t mantissa_product;

    uint32_t final_sign;
    int32_t final_exp;
    uint32_t final_mantissa;
    SC_HAS_PROCESS(FMA);
    FMA(sc_module_name nm, uint32_t mant_bits, uint32_t exp_bits, int rm);
    void run();
    void clearFlags();
    void extractAll();
    void buildParts();
    bool multiplyExact();
    void alignAddend();
    void main_funct();
    void convertPart(uint32_t ein, uint32_t &fin, int32_t &eout);
    bool round_mantissa(uint32_t &mantissa, uint32_t &exp, uint32_t &par_sign, bool guard, bool sticky);
    bool increase_mantissa_by_one(uint32_t &mantissa, uint32_t &exp, uint32_t sign);
    void write_overflow(uint32_t par_sign);
    void write_underflow(uint32_t par_sign);
    bool is_inf(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa);
    bool is_nan(uint32_t exponent_value, uint32_t exponent_bits, uint32_t mantissa);
};