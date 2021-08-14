#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

uint32_t gpu_load_32(uint32_t address) {
  switch(address) {
    case 0x1f801814:
      return(0x1c000000);
    case 0x1f801810:
      return(0);
    default:
      printf("Unknown GPU register: 0x%08x\n", address);
      exit(1);
  }
}

void gpu_store_32(uint32_t address, uint32_t value) {
  switch(address) {
    case 0x1f801810:
      printf("GP0: %08x\n", value);
      return;
    case 0x1f801814:
      printf("GP1: %08x\n", value);
      return;
    default:
      printf("Unknown GPU register: 0x%08x\n", address);
      exit(1);
  }
}

 memory_accessor_t gpu_accessor = {
  .load_32 = gpu_load_32,
  .load_16 = memory_dummy_load_16,
  .load_8 = memory_dummy_load_8,
  .store_32 = gpu_store_32,
  .store_16 = memory_dummy_store_16,
  .store_8 = memory_dummy_store_8,
};
