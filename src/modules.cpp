#include <systemc>
#include "../include/modules.hpp"

extern "C" struct Result run_simulation(uint32_t cycles, const char *tracefile,
                                        uint8_t sizeExponent, uint8_t sizeMantissa, uint8_t roundMode,
                                        uint32_t numRequests, struct Request *requests)
{
    Result result;
    return result;
}

// Note that we need this default sc_main implementation.
int sc_main(int argc, char *argv[])
{
    std::cout << "ERROR" << std::endl;
    return 1;
}