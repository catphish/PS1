#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

typedef struct memory_t {
  uint8_t ram[1024*2048];
  uint8_t rom[1024*512];
  uint32_t dummy;
} memory_t;

void memory_load_bios();

uint32_t memory_load_32(uint32_t address);
uint16_t memory_load_16(uint32_t address);
uint8_t memory_load_8(uint32_t address);

void memory_store_32(uint32_t address, uint32_t value);
void memory_store_16(uint32_t address, uint16_t value);
void memory_store_8(uint32_t address, uint8_t value);

typedef struct memory_accessor_t {
  uint32_t (*load_32)(uint32_t address);
  uint16_t (*load_16)(uint32_t address);
  uint8_t (*load_8)(uint32_t address);
  void (*store_32)(uint32_t address, uint32_t value);
  void (*store_16)(uint32_t address, uint16_t value);
  void (*store_8)(uint32_t address, uint8_t value);
} memory_accessor_t;

#endif