#include <systemc>
#include <systemc.h>
#include <stdint.h>
#include <structs.h>
#include "../include/AddSub.hpp"
#include "../include/Mul.hpp"
#include "../include/Max.hpp"
#include "../include/Min.hpp"


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

    // Clock-gating signals
    sc_signal<bool> clk_addsub;
    sc_signal<bool> clk_mul;
    sc_signal<bool> clk_min;
    sc_signal<bool> clk_max;

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


    SC_HAS_PROCESS(FLOATING_POINT_UNIT);
    FLOATING_POINT_UNIT(sc_module_name name, uint32_t e_bits, uint32_t m_bits, uint32_t roption)
    : sc_module(name), exponent_bits(e_bits), mantissa_bits(m_bits), round_mode(roption)
    ,addsub("addsub",e_bits,m_bits, roption)
    ,mul("mul",e_bits, m_bits, roption)
    ,min_mod("min_mod",e_bits, m_bits)
    ,max_mod("max_mod",e_bits, m_bits){

        // Clock-gating generator
        SC_METHOD(gate_clocks);
        sensitive << clk << op;

        // Bind gated clocks to sub-modules
        addsub.clk(clk_addsub);
        mul.clk(clk_mul);
        min_mod.clk(clk_min);
        max_mod.clk(clk_max);

        // Bind COMMON inputs to all sub-modules
        addsub.r1(r1);addsub.r2(r2);;addsub.op(op);
        mul.r1(r1);mul.r2(r2);
        min_mod.r1(r1);min_mod.r2(r2);
        max_mod.r1(r1);max_mod.r2(r2);

        // Bind each sub-module's outputs to internal signals
        addsub.ro(ro_addsub);
        addsub.zero(zero_addsub);
        addsub.sign(sign_addsub);
        addsub.overflow(overflow_addsub);
        addsub.underflow(underflow_addsub);
        addsub.inexact(inexact_addsub);
        addsub.nan(nan_addsub);

        mul.ro(ro_mul);
        mul.zero(zero_mul);
        mul.sign(sign_mul);
        mul.overflow(overflow_mul);
        mul.underflow(underflow_mul);
        mul.inexact(inexact_mul);
        mul.nan(nan_mul);

        min_mod.ro(ro_min);
        min_mod.zero(zero_min);
        min_mod.sign(sign_min);
        min_mod.overflow(overflow_min);
        min_mod.underflow(underflow_min);
        min_mod.inexact(inexact_min);
        min_mod.nan(nan_min);

        max_mod.ro(ro_max);
        max_mod.zero(zero_max);
        max_mod.sign(sign_max);
        max_mod.overflow(overflow_max);
        max_mod.underflow(underflow_max);
        max_mod.inexact(inexact_max);
        max_mod.nan(nan_max);

        // Use clocked thread for output multiplexer
        SC_CTHREAD(exec, clk.pos());
    }

    void gate_clocks(){
        bool c = clk.read();
        uint32_t code = op.read().to_uint();
        clk_addsub.write(c && (code == 8 || code == 9));
        clk_mul.write(c && (code == 10));
        clk_min.write(c && (code == 13));
        clk_max.write(c && (code == 14));
    }

    void exec(){
        while(true){
            uint32_t code = op.read().to_uint();
            if(code==8 || code == 9){
                ro.write(ro_addsub.read());
                zero.write(zero_addsub.read());
                sign.write(sign_addsub.read());
                overflow.write(overflow_addsub.read());
                underflow.write(underflow_addsub.read());
                inexact.write(inexact_addsub.read());
                nan.write(nan_addsub.read());

            }else if(code == 10){
                ro.write(ro_mul.read());
                zero.write(zero_mul.read());
                sign.write(sign_mul.read());
                overflow.write(overflow_mul.read());
                underflow.write(underflow_mul.read());
                inexact.write(inexact_mul.read());
                nan.write(nan_mul.read());

            }else if(code == 13){
                ro.write(ro_min.read());
                zero.write(zero_min.read());
                sign.write(sign_min.read());
                overflow.write(overflow_min.read());
                underflow.write(underflow_min.read());
                inexact.write(inexact_min.read());
                nan.write(nan_min.read());

            }else if(code == 14){
                ro.write(ro_max.read());
                zero.write(zero_max.read());
                sign.write(sign_max.read());
                overflow.write(overflow_max.read());
                underflow.write(underflow_max.read());
                inexact.write(inexact_max.read());
                nan.write(nan_max.read());
            }else{
                ro.write(0);
                zero.write(false);
                sign.write(false);
                overflow.write(false);
                underflow.write(false);
                inexact.write(false);
                nan.write(false);
            }
            wait();
        }
    }
};
