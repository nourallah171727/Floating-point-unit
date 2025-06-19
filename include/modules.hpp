#include <systemc>

using namespace sc_core;
struct Request {
  uint32_t r1, r2, r3, ro;
  uint8_t op;
};

extern "C" int run_simulation();

