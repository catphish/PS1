#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include "memory.h"

typedef struct dma_t {
  uint32_t control;
} dma_t;

extern struct memory_accessor_t dma_memory_accessor;

void dma_reset();

#endif
