#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

typedef struct memory_accessor_t {
  uint32_t (*load_32)(uint32_t address);
  uint16_t (*load_16)(uint32_t address);
  uint8_t (*load_8)(uint32_t address);
  void (*store_32)(uint32_t address, uint32_t value);
  void (*store_16)(uint32_t address, uint16_t value);
  void (*store_8)(uint32_t address, uint8_t value);
} memory_accessor_t;

extern memory_accessor_t ram_accessor;
extern memory_accessor_t rom_accessor;
extern memory_accessor_t scratchpad_accessor;
extern memory_accessor_t expansion_accessor;
extern memory_accessor_t memory_control_accessor;
extern memory_accessor_t interrupt_accessor;
extern memory_accessor_t dma_accessor;
extern memory_accessor_t timers_accessor;
extern memory_accessor_t cdrom_accessor;
extern memory_accessor_t gpu_accessor;
extern memory_accessor_t spu_accessor;

uint32_t memory_load_32(uint32_t address);
uint16_t memory_load_16(uint32_t address);
uint8_t memory_load_8(uint32_t address);

void memory_store_32(uint32_t address, uint32_t value);
void memory_store_16(uint32_t address, uint16_t value);
void memory_store_8(uint32_t address, uint8_t value);

uint32_t memory_dummy_load_32(uint32_t address);
uint16_t memory_dummy_load_16(uint32_t address);
uint8_t memory_dummy_load_8(uint32_t address);
void memory_dummy_store_32(uint32_t address, uint32_t value);
void memory_dummy_store_16(uint32_t address, uint16_t value);
void memory_dummy_store_8(uint32_t address, uint8_t value);

#endif