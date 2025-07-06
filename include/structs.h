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

#ifdef __cplusplus
extern "C" {
#endif

struct Result run_simulation(uint32_t cycles,
                            const char* tracefile,
                            uint8_t sizeExponent,
                            uint8_t sizeMantissa,
                            uint8_t roundMode,
                            uint32_t numRequests,
                            struct Request* requests);

#ifdef __cplusplus
}
#endif
