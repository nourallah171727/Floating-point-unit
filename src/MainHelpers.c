#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>


//convert args to uints:
uint8_t convert_str_8(const char *str){
    char *endptr;
    errno=0;
    //strtoul is safer than strtol because we're dealing with unsigned ints
    unsigned long ans = strtoul(str,&endptr,0);

    if (errno==ERANGE || endptr==str || *endptr!='\0' || ans>UINT8_MAX) {
        printf("Invalid uint8_t input: %s\n", str);
        exit(1);
    }

    return (uint8_t)ans;
}

uint32_t convert_str_32(const char *str){
    char *endptr;
    errno=0;
    //strtoul is safer than strtol because we're dealing with unsigned ints
    unsigned long ans = strtoul(str,&endptr,0);

    if (errno==ERANGE || endptr==str || *endptr!='\0' || ans>UINT32_MAX) {
        printf("Invalid uint32_t input: %s\n", str);
        exit(1);
    }

    return (uint32_t)ans;
}

void printHelp(){
    printf("The available Options are:\n");
    printf("--help, --tf, --cycles, --size-exponent, --size-mantissa, --round-mode\n");
    printf("This program will be terminated...\n");
}
