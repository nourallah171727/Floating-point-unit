#include <systemc>
#include "../include/modules.hpp"

extern "C" int run_simulation() {
    return 4;
}

// Note that we need this default sc_main implementation.
int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR" << std::endl;
    return 1;
}