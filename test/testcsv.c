#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/structs.h"
#include "../include/MainHelpers.h"

int main() {
    uint32_t count = 0;
    struct Request* reqs = load_csv_requests("test/test.csv", &count);

    
    printf("Parsed %u requests:\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        printf("Request %u => op=%u r1=0x%08x r2=0x%08x r3=0x%08x\n",
            i, reqs[i].op, reqs[i].r1, reqs[i].r2, reqs[i].r3);
    }

    free(reqs);
    return 0; 
}
//for testing : gcc -std=c17 -Iinclude -o testcsv test/testcsv.c src/MainHelpers.c -lm
// ./testcsv

