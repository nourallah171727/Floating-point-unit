#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/structs.h"
#include "../include/MainHelpers.h"

int main() {
    uint32_t sizeExponent = 7;
    uint32_t sizeMantissa = 24;
    uint32_t roundMode = 2;
    uint32_t cycles = 10;  // Max number of requests to simulate

    uint32_t totalRequests = 0;
    struct Request* reqs = load_csv_requests("test/test.csv", sizeExponent, sizeMantissa, roundMode, &totalRequests, cycles);
    printf("Total parsed requests: %u (showing up to %u)\n", totalRequests, cycles);
    uint32_t displayed = (totalRequests < cycles) ? totalRequests : cycles;

    for (uint32_t i = 0; i < displayed; ++i) {
        printf("Request %u => op=%u r1=0x%08x r2=0x%08x r3=0x%08x\n",
            i, reqs[i].op, reqs[i].r1, reqs[i].r2, reqs[i].r3);
    }

    free(reqs);
    return 0;
}
