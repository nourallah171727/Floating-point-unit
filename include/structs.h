#include <stdint.h>

struct Request {
    uint32_t r1, r2, r3, ro;
    uint8_t op;
};

struct Result {
    uint32_t cycles;
    uint32_t signs;
    uint32_t overflows;
    uint32_t underflows;
    uint32_t inexactes;
    uint32_t nans;
};
