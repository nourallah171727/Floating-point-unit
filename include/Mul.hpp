#ifndef MUL_HPP
#define MUL_HPP

#include <systemc>
using namespace sc_core;

SC_MODULE(Mul){
    //in/out ports
    sc_in<uint32_t>r1,r2;
    sc_out<uint32_t>ro;
    sc_out<bool>zero, overflow, underflow, inexact, nan,sign ;

    //parameters
    uint32_t mantissa_bits;
    uint32_t exponent_bits;
    uint32_t round_mode;
    int32_t bias;

    //extracted fields
    uint32_t signa, signb;
    int32_t exp1, exp2;
    uint32_t mantissa1, mantissa2;

    //Constructor
    SC_HAS_PROCESS(Mul);

    Mul(sc_module_name name,uint32_t mantissa_bitsp,uint32_t exponent_bitsp,uint32_t round_modep);

    //Methods
    void exec();
    void extract();
    void multiply();
    uint32_t multiply_and_round(uint64_t m1, uint64_t m2,int32_t &e1, int32_t &e2,bool &is_exact);
    bool handleSpecialCases();
    void writeNaN();

};

#endif //MUL_HPP
