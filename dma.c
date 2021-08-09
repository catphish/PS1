#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "dma.h"
#include "memory.h"

dma_t dma;

void dma_reset() {
  dma.control = 0x07654321;
}

void dma_store_32(uint32_t address, uint32_t value) {
  switch(address - 0x1F801080) {
    case 0x70:
      dma.control = value;
    default:
      printf("Unknown DMA register: 0x%04x (0x%08x)\n", address - 0x1F801080, address);
      exit(1);
  }
}

memory_accessor_t dma_memory_accessor = {
  .store_32 = dma_store_32
};
