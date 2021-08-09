#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "memory.h"

memory_accessor_t * memory_decode_address(uint32_t address) {
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      return(&ram_accessor);
    case 0x1F800000 ... 0x1F8003FF:;
    case 0x8F800000 ... 0x8F8003FF:;
    case 0xAF800000 ... 0xAF8003FF:;
      return(&scratchpad_accessor);
    case 0x1FC00000 ... 0x1FC7FFFF:;
    case 0x9FC00000 ... 0x9FC7FFFF:;
    case 0xBFC00000 ... 0xBFC7FFFF:;
      return(&rom_accessor);
    case 0x1F000000 ... 0x1F0800FF:;
    case 0x1F802000 ... 0x1F803FFF:;
      return(&expansion_accessor);
    case 0x1F801000 ... 0x1F801023:;
    case 0x1F801060 ... 0x1F801063:;
    case 0xFFFE0130 ... 0xFFFE0133:;
      return(&memory_control_accessor);
    case 0x1F801C00 ... 0x1F801FFF:;
      return(&spu_accessor);
    case 0x1F801070 ... 0x1F801077:;
      return(&interrupt_accessor);
    case 0x1F801080 ... 0x1F8010FF:;
      return(&dma_accessor);
    case 0x1F801100 ... 0x1F80112F:;
      return(&timers_accessor);
    case 0x1F801810 ... 0x1F801817:;
      return(&gpu_accessor);
    default:
      printf("Invalid memory access: 0x%08x\n", address);
      exit(1);
  }
}

uint32_t memory_load_32(uint32_t address) {
  if(address % 4) {
    cpu_exception(4);
    return(0);
  }
  return memory_decode_address(address)->load_32(address);
}
uint16_t memory_load_16(uint32_t address) {
  if(address % 2) {
    cpu_exception(4);
    return(0);
  }
  return memory_decode_address(address)->load_16(address);
}
uint8_t memory_load_8(uint32_t address) {
  return memory_decode_address(address)->load_8(address);
}

void memory_store_32(uint32_t address, uint32_t value) {
  if(address % 4) {
    cpu_exception(5);
    return;
  }
  return memory_decode_address(address)->store_32(address, value);
}
void memory_store_16(uint32_t address, uint16_t value) {
  if(address % 2) {
    cpu_exception(5);
    return;
  }
  return memory_decode_address(address)->store_16(address, value);
}
void memory_store_8(uint32_t address, uint8_t value) {
  return memory_decode_address(address)->store_8(address, value);
}

uint32_t memory_dummy_load_32(uint32_t address) {
  printf("Ignoring 32-bit read from 0x%08x\n", address);
  return(0);
}
uint16_t memory_dummy_load_16(uint32_t address) {
  printf("Ignoring 16-bit read from 0x%08x\n", address);
  return(0);
}
uint8_t memory_dummy_load_8(uint32_t address) {
  printf("Ignoring 8-bit read from 0x%08x\n", address);
  return(0);
}
void memory_dummy_store_32(uint32_t address, uint32_t value) {
  printf("Ignoring 32-bit write to 0x%08x\n", address);
}
void memory_dummy_store_16(uint32_t address, uint16_t value) {
  printf("Ignoring 16-bit write to 0x%08x\n", address);
}
void memory_dummy_store_8(uint32_t address, uint8_t value) {
  printf("Ignoring 8-bit write to 0x%08x\n", address);
}
