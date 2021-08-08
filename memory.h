#include <stdint.h>

void memory_load_bios();

uint32_t memory_load_32(uint32_t address);
uint16_t memory_load_16(uint32_t address);
uint8_t memory_load_8(uint32_t address);

void memory_store_32(uint32_t address, uint32_t value);
void memory_store_16(uint32_t address, uint16_t value);
void memory_store_8(uint32_t address, uint8_t value);
