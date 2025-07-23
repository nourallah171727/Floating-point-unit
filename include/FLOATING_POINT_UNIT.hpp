#include <systemc>
#include <systemc.h>
#include <stdint.h>
#include <structs.h>
#include "../include/AddSub.hpp"
#include "../include/Mul.hpp"
#include "../include/Max.hpp"
#include "../include/Min.hpp"
#include "../include/FMA.hpp"


using namespace sc_core;

// TODO: SYSTEMC Modules
SC_MODULE(FLOATING_POINT_UNIT){
    sc_in<uint32_t> r1,r2,r3;
    sc_in<sc_bv<4>> op;
    sc_in<bool> clk;

    sc_out<uint32_t> ro;
    sc_out<bool> zero,sign,overflow,underflow,inexact,nan;

    //parameters
    uint32_t mantissa_bits;
    uint32_t exponent_bits;
    uint32_t round_mode;

    // Sub-module instances
    AddSub addsub;
    Mul mul;
    Min min_mod;
    Max max_mod;
	FMA fma;

    // Clock-gating signals
    sc_signal<bool> clk_addsub;
    sc_signal<bool> clk_mul;
    sc_signal<bool> clk_min;
    sc_signal<bool> clk_max;
    sc_signal<bool> clk_fma;


    // Internal outputs and flags from sub-modules
    sc_signal<uint32_t> ro_addsub;
    sc_signal<bool> zero_addsub;
    sc_signal<bool> sign_addsub;
    sc_signal<bool> overflow_addsub;
    sc_signal<bool> underflow_addsub;
    sc_signal<bool> inexact_addsub;
    sc_signal<bool> nan_addsub;

    sc_signal<uint32_t> ro_mul;
    sc_signal<bool> zero_mul;
    sc_signal<bool> sign_mul;
    sc_signal<bool> overflow_mul;
    sc_signal<bool> underflow_mul;
    sc_signal<bool> inexact_mul;
    sc_signal<bool> nan_mul;

    sc_signal<uint32_t> ro_min;
    sc_signal<bool> zero_min;
    sc_signal<bool> sign_min;
    sc_signal<bool> overflow_min;
    sc_signal<bool> underflow_min;
    sc_signal<bool> inexact_min;
    sc_signal<bool> nan_min;

    sc_signal<uint32_t> ro_max;
    sc_signal<bool> zero_max;
    sc_signal<bool> sign_max;
    sc_signal<bool> overflow_max;
    sc_signal<bool> underflow_max;
    sc_signal<bool> inexact_max;
    sc_signal<bool> nan_max;

    sc_signal<uint32_t> ro_fma;
    sc_signal<bool> zero_fma;
    sc_signal<bool> sign_fma;
    sc_signal<bool> overflow_fma;
    sc_signal<bool> underflow_fma;
    sc_signal<bool> inexact_fma;
    sc_signal<bool> nan_fma;

    SC_HAS_PROCESS(FLOATING_POINT_UNIT);
    FLOATING_POINT_UNIT(sc_module_name name, uint32_t e_bits, uint32_t m_bits, uint32_t roption);

    void gate_clocks();
    void exec();
	uint32_t getPositiveInf();
	uint32_t getNegativeInf();
	double getMax();
	double getMin();

};
