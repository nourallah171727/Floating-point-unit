#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "../include/structs.h"
#include "../include/MainHelpers.h"


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

uint32_t parse_operand(const char* str) {
    if (str == NULL || strlen(str) == 0) {
        return 0;
    }
    char clean[64];
    strncpy(clean, str, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    int len = strlen(clean);
    if (len > 0 && (clean[len - 1] == '\n' || clean[len - 1] == '\r')) {
        clean[len - 1] = '\0';
    }
    if (strncmp(clean, "0x", 2) == 0 || strncmp(clean, "0X", 2) == 0) {
        char* endptr;
        uint32_t value = strtoul(clean, &endptr, 16);
        if (*endptr != '\0') {
            fprintf(stderr, "Invalid hex operand: %s\n", clean);
            exit(1);
        }
        return value;
    }
    char* endptr;
    float fval = strtof(clean, &endptr);
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid float operand: %s\n", clean);
        exit(1);
    }
    uint32_t bits;
    memcpy(&bits, &fval, sizeof(bits));
    return bits;
}

struct Request parse_csv_line(char* line) {
    struct Request request = {0};  
    if (line[0] == '\0' || strspn(line, " \t\n\r") == strlen(line)) {
        fprintf(stderr, "Empty or whitespace-only line is not allowed.\n");
        exit(1);
    }
    char temp_line[512];
    strncpy(temp_line, line, sizeof(temp_line) - 1);
    temp_line[sizeof(temp_line) - 1] = '\0';
    char* lookahead = strtok(temp_line, ",");
    if (!lookahead) goto format_error;
    uint8_t op = (uint8_t)strtoul(lookahead, NULL, 10); // extracting op as int in a copy line to check later for number of commas 
    if (op != 8 && op != 9 && op != 10 && op != 13 && op != 14 && op != 15) {
        fprintf(stderr, "Unsupported opcode: %d\n", op);
        exit(1);
    }
    int comma_count = 0;
    for (char* c = line; *c; c++) {
        if (*c == ',') comma_count++;
    } // counting ',' 
    if (op == 15 && comma_count < 3) {
        fprintf(stderr, "Malformed CSV line (FMA must have at least 4 fields): %s\n", line);
        exit(1);
    }
    if (op != 15 && comma_count < 2) {
        fprintf(stderr, "Malformed CSV line (expected at least op,r1,r2): %s\n", line);
        exit(1);
    }
    char* field = strtok(line, ","); 
    if (!field) goto format_error;
    request.op = (uint8_t)strtoul(field, NULL, 10);  // parse op as decimal int
    field = strtok(NULL, ",");
    if (!field) goto format_error;
    request.r1 = parse_operand(field);  
    field = strtok(NULL, ",");
    if (!field) goto format_error;
    request.r2 = parse_operand(field);  
    field = strtok(NULL, ",");
    if (request.op == 15 && (!field || strlen(field) == 0)) {
        fprintf(stderr, "FMA requires a non-empty r3 operand.\n");
        exit(1); // r1 , r2 and r3 parsing 
    }
    request.r3 = (field ? parse_operand(field) : 0);  // parse r3 or use 0 if empty
    return request;  // return filled Request struct
format_error:
    fprintf(stderr, "Malformed CSV line. Expected format: op,r1,r2,r3\n");
    exit(1);
}

struct Request* load_csv_requests(const char* filename, uint32_t* out_count) {
    printf("Trying to open: %s\n", filename); // for debug 
    FILE* file = fopen(filename, "r");  // open file
    if (!file) {
        perror("Error opening CSV file"); // throw error 
        exit(1);
    }

    char line[512];  
    int capacity = 32;  
    int count = 0;      
    struct Request* array = malloc(sizeof(struct Request) * capacity); //allocate mem

    int is_first_line = 1;

    while (fgets(line, sizeof(line), file)) {
        if (is_first_line) { 
            is_first_line = 0;
            if (strncmp(line, "op,r1,r2,r3", strlen("op,r1,r2,r3")) != 0) {
               fprintf(stderr, "Missing or invalid CSV header. Expected: op,r1,r2,r3\n");
               exit(1);
               }
            continue;
        }

        if (line[0] == '\0' || strspn(line, " \t\n\r") == strlen(line)) {
            fprintf(stderr, "Error: Empty or whitespace-only line.\n");
            exit(1);
        }

        if (count >= capacity) {
            capacity *= 2; 
            array = realloc(array, sizeof(struct Request) * capacity); // double when needed 
        }

        struct Request next = parse_csv_line(line);  
        array[count++] = next;  
    }

    fclose(file);  
    *out_count = count;  
    return array;  
}

