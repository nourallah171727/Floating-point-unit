#include <systemc>

using namespace sc_core;
struct Request {
  uint32_t r1, r2, r3, ro;
  uint8_t op;
};
 struct Result {
 uint32 t cycles ;
 uint32 t signs ;
 uint32 t overflows ;
 uint32 t underflows ;
 uint32 t inextacts ;
 uint32 t nans;
 };

extern "C" int run_simulation();

