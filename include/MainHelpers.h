#include <stdint.h>
#include "structs.h"

uint8_t convert_str_8(const char* str);
uint32_t convert_str_32(const char* str);
void printHelp(void);
uint32_t parse_operand(const char* str,uint32_t sizeMantissa, uint32_t sizeExponent, uint32_t roundMode);
struct Request parse_csv_line(char* line, uint32_t sizeMantissa, uint32_t sizeExponent, uint32_t roundMode);
struct Request* load_csv_requests(const char* filename, uint32_t sizeExponent, uint32_t sizeMantissa, uint32_t roundMode,uint32_t* out_count,uint32_t cycles);
uint32_t custom_parse(char* clean , uint32_t sizeMantissa, uint32_t sizeExponent, uint32_t roundMode );

