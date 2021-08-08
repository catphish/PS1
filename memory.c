#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "memory.h"

memory_t memory;

void memory_load_bios() {
  int fd = open("BIOS/ps-22a.bin", O_RDONLY);
  int bytes = read(fd, memory.rom, 524288);
  if(bytes != 524288) {
    printf("memory.rom READ FAILED!\n");
    exit(1);
  }
  close(fd);
}

uint32_t memory_load_32(uint32_t address) {
  if(address % 4) {
    cpu_exception(4);
    return(0);
  }
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      return *(uint32_t*)(memory.ram + (address & 0x1FFFFF));
    case 0x1FC00000 ... 0x1FC7FFFF:;
    case 0x9FC00000 ... 0x9FC7FFFF:;
    case 0xBFC00000 ... 0xBFC7FFFF:;
      return *(uint32_t*)(memory.rom + (address & 0x7FFFF));
    case 0x1F801814:; // GPU Status
      return(1<<28);
    default:
      return(0);
  }
}

uint16_t memory_load_16(uint32_t address) {
  if(address % 2) {
    cpu_exception(4);
    return(0);
  }
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      return *(uint16_t*)(memory.ram + (address & 0x1FFFFF));
      break;
    case 0x1FC00000 ... 0x1FC7FFFF:;
    case 0x9FC00000 ... 0x9FC7FFFF:;
    case 0xBFC00000 ... 0xBFC7FFFF:;
      return *(uint16_t*)(memory.rom + (address & 0x7FFFF));
      break;
    default:
      return(0);
  }
}

uint8_t memory_load_8(uint32_t address) {
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      return *(uint8_t*)(memory.ram + (address & 0x1FFFFF));
      break;
    case 0x1FC00000 ... 0x1FC7FFFF:;
    case 0x9FC00000 ... 0x9FC7FFFF:;
    case 0xBFC00000 ... 0xBFC7FFFF:;
      return *(uint8_t*)(memory.rom + (address & 0x7FFFF));
      break;
    default:
      return(0);
  }
}

void memory_store_32(uint32_t address, uint32_t value) {
  if(address % 4) {
    cpu_exception(5);
    return;
  }
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      if(cpu.cop0.sr & (1<<16)) {
        // Ignoring write while cache is isolated
      } else
        *(uint32_t*)(memory.ram + (address & 0x1FFFFF)) = value;
      break;
    default:
      break;
  }
}

void memory_store_16(uint32_t address, uint16_t value) {
  if(address % 2) {
    cpu_exception(5);
    return;
  }
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      if(cpu.cop0.sr & (1<<16)) {
        // Ignoring write while cache is isolated
      } else
        *(uint16_t*)(memory.ram + (address & 0x1FFFFF)) = (uint16_t)value;
      break;
    default:
      break;
  }
}

void memory_store_8(uint32_t address, uint8_t value) {
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      if(cpu.cop0.sr & (1<<16)) {
        // Ignoring write while cache is isolated
      } else
        *(uint8_t*)(memory.ram + (address & 0x1FFFFF)) = (uint8_t)value;
      break;
    default:
      break;
  }
}
