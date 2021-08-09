#include <stdint.h>
#include "cpu.h"
#include "memory.h"

uint8_t ram[1024*2048];

uint32_t ram_load_32(uint32_t address) {
  return *(uint32_t*)(ram + (address & 0x1FFFFF));
}
uint16_t ram_load_16(uint32_t address) {
  return *(uint16_t*)(ram + (address & 0x1FFFFF));
}
uint8_t ram_load_8(uint32_t address) {
  return *(uint8_t*)(ram + (address & 0x1FFFFF));
}
void ram_store_32(uint32_t address, uint32_t value) {
  if((cpu.cop0_registers.sr & (1<<16)) == 0)
    *(uint32_t*)(ram + (address & 0x1FFFFF)) = value;
}
void ram_store_16(uint32_t address, uint16_t value) {
  if((cpu.cop0_registers.sr & (1<<16)) == 0)
    *(uint16_t*)(ram + (address & 0x1FFFFF)) = value;
}
void ram_store_8(uint32_t address, uint8_t value) {
  if((cpu.cop0_registers.sr & (1<<16)) == 0)
    *(uint8_t*)(ram + (address & 0x1FFFFF)) = value;
}
 memory_accessor_t ram_accessor = {
  .load_32 = ram_load_32,
  .load_16 = ram_load_16,
  .load_8 = ram_load_8,
  .store_32 = ram_store_32,
  .store_16 = ram_store_16,
  .store_8 = ram_store_8,
};
