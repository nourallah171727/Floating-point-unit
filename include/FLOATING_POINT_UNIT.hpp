#include <systemc>
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

    SC_HAS_PROCESS(FLOATING_POINT_UNIT);
    FLOATING_POINT_UNIT(sc_module_name name, uint32_t e_bits, uint32_t m_bits, uint32_t roption)
    : sc_module(name), exponent_bits(e_bits), mantissa_bits(m_bits), round_mode(roption))

        sensitive << clk.pos();
        SC_METHOD(exec);

    }

    void exec(){
        switch(op.to_uint()){
            case 8:

                break;
            case 9:
                break;
            case 10:
                break;
            case 13:
                break;
            case 14:
                break;
            case 15:
                break;
        }
    }



}