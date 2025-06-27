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
uint32_t parse_operand(const char* str){
    if ( str == NULL || strlen(str)==0){
        return 0; // but only for r3 when uneeded
    }
    if (strlen(str)>2 && str[0]=='0' && (str[1]=='x' || str[1] == 'X')) { // hexa 
        char* endptr; 
        errno = 0; 
        uint32_t value = strtoul(str, &endptr, 16); // unsigned long for safety if inf then it will be treated by the other FPU modules 
        if (errno != 0 || *endptr != '\0') {
            fprintf(stderr, "Invalid hex operand: %s\n", str);
            exit(1);
        }
        return value;
    }
    char* endptr;
    errno = 0;
    float fval = strtof(str, &endptr);
    if (errno != 0 || endptr == str || *endptr != '\0') {
        fprintf(stderr, "Invalid float operand: %s\n", str);
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
    int comma_count = 0;
    for (char* c = line; *c; c++) {
        if (*c == ',') comma_count++;
    } // counting ',' 
    if ((op == 15 && comma_count != 3) || (op != 15 && comma_count != 2)) {
        fprintf(stderr, "Malformed CSV line (wrong number of fields for op=%d): %s\n", op, line);
        exit(1); // deciding wether it is FMA or not
    }
    char* field = strtok(line, ","); 
    if (!field) goto format_error;
    request.op = (uint8_t)strtoul(field, NULL, 10);  // parse op as decimal int
    if (request.op != 8 && request.op != 9 && request.op != 10 &&
        request.op != 13 && request.op != 14 && request.op != 15) {
        fprintf(stderr, "Unsupported opcode: %d\n", request.op);
        exit(1);
    }
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
            if (strncmp(line, "op,r1,r2,r3", 12) != 0 && strncmp(line, "op,r1,r2,r3\n", 13) != 0) {
                fprintf(stderr, "Missing or invalid CSV header. Expected: op,r1,r2,r3\n");
                exit(1); // first line as it shoud be 
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

