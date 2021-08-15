#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "dma.h"
#include "memory.h"
#include "gpu.h"

extern uint8_t ram[];

struct __attribute__((packed)) {
  struct __attribute__((packed)) {
    uint32_t base_address;
    union {
      uint32_t words;
      struct __attribute__((packed)) {
        uint16_t blocksize;
        uint16_t blocks;
      };
    };
    union {
      uint32_t control_32;
      struct __attribute__((packed)) {
        unsigned int direction : 1;
        unsigned int step : 1;
        unsigned int rfu_0 : 6;
        unsigned int chopping_enable : 1;
        unsigned int sync_mode : 2;
        unsigned int rfu_1 : 5;
        unsigned int chopping_dma_window_size : 3;
        unsigned int rfu_2 : 1;
        unsigned int chopping_cpu_window_size : 3;
        unsigned int rfu_3 : 1;
        unsigned int start_busy : 1;
        unsigned int rfu_4 : 3;
        unsigned int start_trigger : 1;
        unsigned int rfu_5 : 3;
        uint32_t rfu;
      } control;
    };
  } channels[7];
  union {
    uint32_t control_32;
    struct __attribute__((packed)) {
      unsigned int dma_0_priority : 3;
      unsigned int dma_0_enable : 1;
      unsigned int dma_1_priority : 3;
      unsigned int dma_1_enable : 1;
      unsigned int dma_2_priority : 3;
      unsigned int dma_2_enable : 1;
      unsigned int dma_3_priority : 3;
      unsigned int dma_3_enable : 1;
      unsigned int dma_4_priority : 3;
      unsigned int dma_4_enable : 1;
      unsigned int dma_5_priority : 3;
      unsigned int dma_5_enable : 1;
      unsigned int dma_6_priority : 3;
      unsigned int dma_6_enable : 1;
      unsigned int rfu_6 : 4;
    } control;
  };
  union {
    uint32_t interrupt_32;
    struct __attribute__((packed)) {
      unsigned int rfu_7 : 15;
      unsigned int force_irq : 1;
      unsigned int irq_enable_0 : 1;
      unsigned int irq_enable_1 : 1;
      unsigned int irq_enable_2 : 1;
      unsigned int irq_enable_3 : 1;
      unsigned int irq_enable_4 : 1;
      unsigned int irq_enable_5 : 1;
      unsigned int irq_enable_6 : 1;
      unsigned int irq_master_enable : 1;
      unsigned int irq_flag_0 : 1;
      unsigned int irq_flag_1 : 1;
      unsigned int irq_flag_2 : 1;
      unsigned int irq_flag_3 : 1;
      unsigned int irq_flag_4 : 1;
      unsigned int irq_flag_5 : 1;
      unsigned int irq_flag_6 : 1;
      unsigned int irq_master_flag : 1;
    } interrupt;
  };
} dma;

void dma_reset() {
  dma.control_32 = 0x07654321;
}

void otc_dma_transfer() {
  if(dma.channels[6].control_32 == 0x11000002) {
    //printf("DMA OTC transfer starting!\n");
    uint32_t words = dma.channels[6].words - 1;
    uint32_t address = dma.channels[6].base_address & 0x1fffff;
    uint32_t end_address = address - words * 4;
    while(address > end_address) {
      *(uint32_t*)(ram + address) = address - 4;
      //printf("Setting %08x to %08x\n", address, address-4);
      address -= 4;
    }
    *(uint32_t*)(ram + address) = 0xffffff;
    //printf("Setting %08x to %08x\n", address, 0xffffff);
  } else {
    printf("Unexpected DMA options for OTC transfer!\n");
    exit(1);
  }
  dma.channels[6].control.start_trigger = 0;
  dma.channels[6].control.start_busy = 0;
  //printf("dma transfer complete!\n");
}

void gpu_dma_transfer() {
  if(dma.channels[2].control_32 == 0x01000401) {
    //printf("DMA GPU (linked list) transfer starting!\n");
    uint32_t address = dma.channels[2].base_address & 0x1fffff;
    //printf("Base: %08x\n", address);
    while(1) {
      uint32_t header = *(uint32_t*)(ram + address);
      //printf("Address: %08x Header: %08x\n", address, header);
      uint32_t packet_size = header >> 24;
      for(uint32_t n=0; n<packet_size*4; n+=4) {
        uint32_t command = *(uint32_t*)(ram + address + n + 4);
        gpu_gp0(command);
      }
      if(header & 0x800000)
        break;
      address = header & 0x1fffff;
    }
  } else if (dma.channels[2].control_32 == 0x01000201) {
    //printf("DMA GPU (sequential) transfer starting!\n");
    uint32_t address = dma.channels[2].base_address & 0x1fffff;
    uint32_t words = dma.channels[2].blocksize * dma.channels[2].blocks;
    uint32_t end_address = address + words * 4;
    //printf("info: %08x %08x\n", words, address);
    while(address < end_address) {
      uint32_t command = *(uint32_t*)(ram + address);
      gpu_gp0(command);
      address += 4;
    }
  } else {
    printf("Unexpected DMA options for GPU transfer: %08x\n", dma.channels[2].control_32);
    exit(1);
  }
  dma.channels[2].control.start_trigger = 0;
  dma.channels[2].control.start_busy = 0;
  //printf("dma transfer complete!\n");
}

void dummy_dma_transfer() {
  printf("Unsupported DMA channel.\n");
  exit(1);
}

void (*dma_transfer[256])() = {
  dummy_dma_transfer,
  dummy_dma_transfer,
  gpu_dma_transfer,
  dummy_dma_transfer,
  dummy_dma_transfer,
  dummy_dma_transfer,
  otc_dma_transfer
};

void dma_store_32(uint32_t address, uint32_t value) {
  uint32_t reg = address - 0x1F801080;
  *(uint32_t*)((uint8_t*)&dma + reg) = value;

  // printf("Writing %08X to offset %02X\n", value, reg);
  // for(int i=0; i<8; i++) {
  //   for(int j=0; j<4; j++) {
  //     printf("%08X ", ((uint32_t*)&dma)[i*4+j]);
  //   }
  //   printf("\n");
  // }
  // printf("\n");

  uint8_t channel = reg >> 4;
  if(channel < 7) {
    uint8_t trigger = dma.channels[channel].control.start_trigger;
    uint8_t enabled = dma.channels[channel].control.start_busy;
    uint8_t sync_mode = dma.channels[channel].control.sync_mode;
    if(enabled && (trigger || sync_mode)) {
      dma_transfer[channel]();
    }
  }
}

uint32_t dma_load_32(uint32_t address) {
  uint32_t reg = address - 0x1F801080;
  return(*(uint32_t*)((uint8_t*)&dma + reg));
}

 memory_accessor_t dma_accessor = {
  .load_32 = dma_load_32,
  .load_16 = memory_dummy_load_16,
  .load_8 = memory_dummy_load_8,
  .store_32 = dma_store_32,
  .store_16 = memory_dummy_store_16,
  .store_8 = memory_dummy_store_8,
};
