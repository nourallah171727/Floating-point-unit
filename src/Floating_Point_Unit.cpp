#include "../include/FLOATING_POINT_UNIT.hpp"
#include <cmath>
#include <stdint.h>

FLOATING_POINT_UNIT::FLOATING_POINT_UNIT(sc_module_name name, uint32_t e_bits, uint32_t m_bits, uint32_t roption)
    : sc_module(name), exponent_bits(e_bits), mantissa_bits(m_bits), round_mode(roption)
    ,addsub("addsub",e_bits,m_bits, roption)
    ,mul("mul",m_bits, e_bits, roption)
    ,min_mod("min_mod",e_bits, m_bits)
    ,max_mod("max_mod",e_bits, m_bits)
	,fma("fma",m_bits, e_bits, roption){

        // Clock-gating generator
        SC_METHOD(gate_clocks);
        sensitive << clk << op;

        // Bind gated clocks to sub-modules
        addsub.clk(clk_addsub);
        mul.clk(clk_mul);
        min_mod.clk(clk_min);
        max_mod.clk(clk_max);
		fma.clk(clk_fma);

        // Bind COMMON inputs to all sub-modules
        addsub.r1(r1);addsub.r2(r2);;addsub.op(op);
        mul.r1(r1);mul.r2(r2);
        min_mod.r1(r1);min_mod.r2(r2);
        max_mod.r1(r1);max_mod.r2(r2);
		fma.r1(r1);fma.r2(r2);fma.r3(r3);

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

        fma.ro(ro_fma);
        fma.zero(zero_fma);
        fma.sign(sign_fma);
        fma.overflow(overflow_fma);
        fma.underflow(underflow_fma);
        fma.inexact(inexact_fma);
        fma.nan(nan_fma);

        // Use clocked thread for output multiplexer
        SC_METHOD(exec);
           sensitive << op
                     << ro_addsub << zero_addsub << sign_addsub
                     << overflow_addsub << underflow_addsub << inexact_addsub << nan_addsub
                     << ro_mul << zero_mul << sign_mul
                     << overflow_mul << underflow_mul << inexact_mul << nan_mul
                     << ro_min << zero_min << sign_min
                     << overflow_min << underflow_min << inexact_min << nan_min
                     << ro_max << zero_max << sign_max
                     << overflow_max << underflow_max << inexact_max << nan_max
					 << ro_fma << zero_fma << sign_fma
                     << overflow_fma << underflow_fma << inexact_fma << nan_fma;
}

void FLOATING_POINT_UNIT::gate_clocks(){

    bool c = clk.read();
    uint32_t code = op.read().to_uint();
    clk_addsub.write(c && (code == 8 || code == 9));
    clk_mul.write(c && (code == 10));
    clk_min.write(c && (code == 13));
    clk_max.write(c && (code == 14));
	clk_fma.write(c && (code == 15));
}


void FLOATING_POINT_UNIT::exec(){

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
     }else if(code == 15){
       ro.write(ro_fma.read());
       zero.write(zero_fma.read());
       sign.write(sign_fma.read());
       overflow.write(overflow_fma.read());
       underflow.write(underflow_fma.read());
       inexact.write(inexact_fma.read());
       nan.write(nan_fma.read());
     }else{
       ro.write(0);
       zero.write(false);
       sign.write(false);
       overflow.write(false);
       underflow.write(false);
       inexact.write(false);
       nan.write(false);
    }
}

uint32_t FLOATING_POINT_UNIT::getPositiveInf(){
	return (((1U<<exponent_bits)-1)<<mantissa_bits);
}

uint32_t FLOATING_POINT_UNIT::getNegativeInf(){
    return (1U <<(exponent_bits + mantissa_bits))|(((1U << exponent_bits)-1) << mantissa_bits);
}

double FLOATING_POINT_UNIT::getMax(){
    uint32_t exponentMax = (1 << (exponent_bits -1))- 1;      // max exponent value
    double mantissaMax = 2.0 - std::pow(2.0, -mantissa_bits); //alle Mantissa bits auf 1 +1
	return mantissaMax* std::pow(2.0, exponentMax);
}

double FLOATING_POINT_UNIT::getMin(){
    return -getMax();
}





