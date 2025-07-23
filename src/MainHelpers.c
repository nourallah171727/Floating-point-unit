#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "../include/structs.h"
#include "../include/MainHelpers.h"

// convert args to uints:
uint8_t convert_str_8(const char *str)
{
    char *endptr;
    errno = 0;
    // strtoul is safer than strtol because we're dealing with unsigned ints
    unsigned long ans = strtoul(str, &endptr, 0);

    if (errno == ERANGE || endptr == str || *endptr != '\0' || ans > UINT8_MAX)
    {
        printf("Invalid uint8_t input: %s\n", str);
        exit(1);
    }

    return (uint8_t)ans;
}

uint32_t convert_str_32(const char *str)
{
    char *endptr;
    errno = 0;
    // strtoul is safer than strtol because we're dealing with unsigned ints
    unsigned long ans = strtoul(str, &endptr, 0);

    if (errno == ERANGE || endptr == str || *endptr != '\0' || ans > UINT32_MAX)
    {
        printf("Invalid uint32_t input: %s\n", str);
        exit(1);
    }

    return (uint32_t)ans;
}

void printHelp()
{
    printf("The available Options are:\n");
    printf("--help, --tf, --cycles, --size-exponent, --size-mantissa, --round-mode\n");
    printf("This program will be terminated...\n");
}

uint32_t parse_operand(const char *str, uint32_t sizeMantissa, uint32_t sizeExponent, uint32_t roundMode)
{
    if (str == NULL || strlen(str) == 0)
    {
        return 0;
    }
    char clean[64];
    strncpy(clean, str, sizeof(clean) - 1);
    clean[strcspn(clean, "\r\n")] = '\0';
    clean[sizeof(clean) - 1] = '\0';
    int len = strlen(clean);
    if (len > 0 && (clean[len - 1] == '\n' || clean[len - 1] == '\r'))
    {
        clean[len - 1] = '\0';
    }
    if (strncmp(clean, "0x", 2) == 0 || strncmp(clean, "0X", 2) == 0)
    {
        char *endptr;
        uint32_t value = strtoul(clean, &endptr, 16);
        if (*endptr != '\0')
        {
            fprintf(stderr, "Invalid hex operand: %s\n", clean);
            exit(1);
        }
        return value;
    }
    return custom_parse(clean, sizeMantissa, sizeExponent, roundMode);
}
uint32_t custom_parse(char *clean, uint32_t sizeMantissa, uint32_t sizeExponent, uint32_t roundMode)
{
    if (strchr(clean, '.') == NULL)
    {
        char *endptr;
        uint32_t val = (uint32_t)strtoul(clean, &endptr, 10);
        if (*endptr != '\0')
        {
            fprintf(stderr, "Invalid integer input: %s\n", clean);
            exit(1);
        }
        return val; // for decimals directly convert into binary form
    }
    double value = strtod(clean, NULL);
    if (value == 0.0)
    {
        return 0;
    }
    uint32_t sign = (value < 0) ? 1 : 0;
    value = fabs(value);
    int exponent;
    double normalized = frexp(value, &exponent);
    normalized *= 2.0;
    exponent -= 1;
    int bias = (1 << (sizeExponent - 1)) - 1;
    int biased_exp = exponent + bias;
    if (biased_exp <= 0)
    {
        return 0;
    }
    if (biased_exp >= (1 << sizeExponent) - 1)
        return (sign << 31) | (((1 << sizeExponent) - 1) << sizeMantissa); // INF

    double frac = normalized - 1.0;
    double scaled = frac * (1 << sizeMantissa);
    uint32_t mantissa = (uint32_t)(scaled);

    double leftover = scaled - mantissa;
    switch (roundMode)
    {
    case 0:
        if (leftover > 0.5 || (leftover == 0.5 && (mantissa & 1)))
            mantissa++;
        break;
    case 1:
        if (leftover >= 0.5)
            mantissa++;
        break;
    case 2:
        break;
    case 3:
        if (sign == 0 && leftover > 0.0)
            mantissa++;
        break;
    case 4:
        if (sign == 1 && leftover > 0.0)
            mantissa++;
        break;
    default:
        break;
    }

    if (mantissa >= (1U << sizeMantissa))
    {
        mantissa = 0;
        biased_exp++;
        if (biased_exp >= (1 << sizeExponent) - 1)
            return (sign << 31) | (((1 << sizeExponent) - 1) << sizeMantissa);
    }

    uint32_t result = (sign << 31) | (biased_exp << sizeMantissa) | mantissa;
    return result;
}

struct Request parse_csv_line(char *line, uint32_t sizeMantissa, uint32_t sizeExponent, uint32_t roundMode)
{
    struct Request request = {0};
    if (line[0] == '\0' || strspn(line, " \t\n\r") == strlen(line))
    {
        fprintf(stderr, "Empty or whitespace-only line is not allowed.\n");
        exit(1);
    }
    char temp_line[512];
    strncpy(temp_line, line, sizeof(temp_line) - 1);
    temp_line[sizeof(temp_line) - 1] = '\0';
    char *lookahead = strtok(temp_line, ",");

    if (!lookahead)
        goto format_error;
    uint8_t op = (uint8_t)strtoul(lookahead, NULL, 10); // extracting op as int in a copy line to check later for number of commas
    if (op != 8 && op != 9 && op != 10 && op != 13 && op != 14 && op != 15)
    {
        fprintf(stderr, "Unsupported opcode: %d\n", op);
        exit(1);
    }

    int comma_count = 0;
    for (char *c = line; *c; c++)
    {
        if (*c == ',')
            comma_count++;
    } // counting ','
    if (op == 15 && comma_count < 3)
    {
        fprintf(stderr, "Malformed CSV line (FMA must have at least 4 fields): %s\n", line);
        exit(1);
    }
    if (op != 15 && comma_count < 2)
    {
        fprintf(stderr, "Malformed CSV line (expected at least op,r1,r2): %s\n", line);
        exit(1);
    }

    char *field = strtok(line, ",");
    if (!field)
        goto format_error;
    request.op = (uint8_t)strtoul(field, NULL, 10); // parse op as decimal int
    field = strtok(NULL, ",");
    if (!field)
        goto format_error;
    request.r1 = parse_operand(field, sizeMantissa, sizeExponent, roundMode);
    field = strtok(NULL, ",");
    if (!field)
        goto format_error;
    request.r2 = parse_operand(field, sizeMantissa, sizeExponent, roundMode);
    field = strtok(NULL, ",");
    if (request.op == 15 && (!field || strlen(field) == 0))
    {
        fprintf(stderr, "FMA requires a non-empty r3 operand.\n");
        exit(1); // r1 , r2 and r3 parsing
    }
    request.r3 = (field ? parse_operand(field, sizeMantissa, sizeExponent, roundMode) : 0); // parse r3 or use 0 if empty
    return request;                                                                         // return filled Request struct
format_error:
    fprintf(stderr, "Malformed CSV line. Expected format: op,r1,r2,r3\n");
    exit(1);
}

struct Request *load_csv_requests(const char *filename,
                                  uint32_t sizeExponent,
                                  uint32_t sizeMantissa,
                                  uint32_t roundMode,
                                  uint32_t *out_count,
                                  uint32_t cycles)
{
    printf("Trying to open: %s\n", filename);
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening CSV file");
        exit(1);
    }

    char line[512];
    int capacity = 32;
    int count = 0;
    struct Request *array = malloc(sizeof(struct Request) * capacity);

    int is_first_line = 1;

    while (fgets(line, sizeof(line), file))
    {
        if (is_first_line)
        {
            is_first_line = 0;
            line[strcspn(line, "\r\n")] = '\0'; // Remove newline
            if (strcmp(line, "op,r1,r2,r3") != 0)
            {
                fprintf(stderr, "Missing or invalid CSV header. Expected: op,r1,r2,r3\n");
                exit(1);
            }
            continue;
        }

        if (line[0] == '\0' || strspn(line, " \t\n\r") == strlen(line))
        {
            fprintf(stderr, "Error: Empty or whitespace-only line.\n");
            exit(1);
        }

        struct Request next = parse_csv_line(line, sizeExponent, sizeMantissa, roundMode);

        if (count < cycles)
        {
            if (count >= capacity)
            {
                capacity *= 2;
                struct Request *temp = realloc(array, sizeof(struct Request) * capacity);
                if (!temp)
                {
                    free(array);
                    fprintf(stderr, "Memory allocation failed.\n");
                    exit(1);
                }
                array = temp;
            }
            array[count] = next;
        }

        count++;
    }

    fclose(file);

    if (count > cycles)
    {
        printf("Only %u out of %u requests will be simulated due to cycle limit.\n", cycles, count);
        *out_count = cycles;
        struct Request *trimmed = malloc(sizeof(struct Request) * cycles);
        memcpy(trimmed, array, sizeof(struct Request) * cycles);
        free(array);
        return trimmed;
    }
    else
    {
        *out_count = count;
    }

    return array;
}
