#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "memory.h"
#include "ram.h"
#include "rom.h"

memory_accessor_t * memory_decode_address(uint32_t address) {
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      return(&ram_accessor);
    case 0x1FC00000 ... 0x1FC7FFFF:;
    case 0x9FC00000 ... 0x9FC7FFFF:;
    case 0xBFC00000 ... 0xBFC7FFFF:;
      return(&rom_accessor);
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
  if(address % 4) {
    cpu_exception(4);
    return(0);
  }
  return memory_decode_address(address)->load_16(address);
}
uint8_t memory_load_8(uint32_t address) {
  if(address % 4) {
    cpu_exception(4);
    return(0);
  }
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
  if(address % 4) {
    cpu_exception(5);
    return;
  }
  return memory_decode_address(address)->store_16(address, value);
}
void memory_store_8(uint32_t address, uint8_t value) {
  if(address % 4) {
    cpu_exception(5);
    return;
  }
  return memory_decode_address(address)->store_8(address, value);
}
