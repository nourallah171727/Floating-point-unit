#include <systemc>
#include "../include/modules.hpp"
#include "../include/structs.h"

extern "C" struct Result run_simulation() {
	struct Result result;

    return result;
}

// Note that we need this default sc_main implementation.
int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR" << std::endl;
    return 1;
}