#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

uint8_t rom[1024*512];

void rom_load_bios() {
  int fd = open("BIOS/ps-22a.bin", O_RDONLY);
  int bytes = read(fd, rom, 524288);
  if(bytes != 524288) {
    printf("rom READ FAILED!\n");
    exit(1);
  }
  close(fd);
}

uint32_t rom_load_32(uint32_t address) {
  return *(uint32_t*)(rom + (address & 0x7FFFF));
}
uint16_t rom_load_16(uint32_t address) {
  return *(uint16_t*)(rom + (address & 0x7FFFF));
}
uint8_t rom_load_8(uint32_t address) {
  return *(uint8_t*)(rom + (address & 0x7FFFF));
}
void rom_store_32(uint32_t address, uint32_t value) {
  printf("Tried to write to ROM: 0x%08x\n", address);
  exit(1);
}
void rom_store_16(uint32_t address, uint16_t value) {
  printf("Tried to write to ROM: 0x%08x\n", address);
  exit(1);
}
void rom_store_8(uint32_t address, uint8_t value) {
  printf("Tried to write to ROM: 0x%08x\n", address);
  exit(1);
}
memory_accessor_t rom_accessor = {
  .load_32 = rom_load_32,
  .load_16 = rom_load_16,
  .load_8 = rom_load_8,
  .store_32 = rom_store_32,
  .store_16 = rom_store_16,
  .store_8 = rom_store_8,
};
