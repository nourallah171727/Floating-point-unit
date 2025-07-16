#include <systemc>
#include "../include/FLOATING_POINT_UNIT.hpp"

extern "C" struct Result run_simulation(uint32_t cycles, const char *tracefile,
                                        uint8_t sizeExponent, uint8_t sizeMantissa, uint8_t roundMode,
                                        uint32_t numRequests, struct Request *requests)
{
    Result result;
    result.cycles = std::min(cycles, numRequests);


    //input signals
    sc_signal<uint32_t> r1,r2,r3;
    sc_signal<sc_bv<4>> op;
    sc_clk clk("clk",1,SC_SEC);

    //output signals
    sc_signal<uint32_t> ro;
    sc_signal<bool> zero,sign,overflow,underflow,inexact,nan;


    //fpu
    FLOATING_POINT_UNIT fpu("FPU",sizeExponent,sizeMantissa,roundMode);
    fpu.clk(clk);
    fpu.r1(r1);
    fpu.r2(r2);
    fpu.r3(r3);
    fpu.op(op);

    fpu.ro(ro);
    fpu.overflow(overflow);
    fpu.underflow(underflow);
    fpu.inexact(inexact);
    fpu.nan(nan);
    fpu.zero(zero);
    fpu.sign(sign);

    //counters for Result
    uint32_t signs=0;
    uint32_t overflows=0;
    uint32_t underflows=0;
    uint32_t inexactes=0;
    uint32_t nans=0;


    int i=0;
    while(i<numRequests && i<cycles){
        struct Request request=requests[i];
        r1.write(request.r1);
        r2.write(request.r2);
        r3.write(request.r3);
        op.write(request.op);

        //run a takt
        sc_start(1,SC_SEC);

        request.ro = ro.read();

        if(sign.read()){
            signs++;
        }
        if(overflow.read()){
            overflows++;
        }
        if(underflow.read()){
            underflows++;
        }
        if(inexact.read()){
            inexacts++;
        }
        if(nan.read()){
            nans++;
        }


        i++;
    }

    result.signs=signs;
    result.overflows=overflows;
    result.underflows=underflows;
    result.inexactes=inexactes;
    result.nans=nan;

    return result;
}

// Note that we need this default sc_main implementation.
int sc_main(int argc, char *argv[])
{
    std::cout << "ERROR" << std::endl;
    return 1;
}