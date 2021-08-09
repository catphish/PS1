#include <stdint.h>
#include "cpu.h"
#include "memory.h"

uint8_t scratchpad[1024];

uint32_t scratchpad_load_32(uint32_t address) {
  return *(uint32_t*)(scratchpad + (address & 0x3FF));
}
uint16_t scratchpad_load_16(uint32_t address) {
  return *(uint16_t*)(scratchpad + (address & 0x3FF));
}
uint8_t scratchpad_load_8(uint32_t address) {
  return *(uint8_t*)(scratchpad + (address & 0x3FF));
}
void scratchpad_store_32(uint32_t address, uint32_t value) {
  *(uint32_t*)(scratchpad + (address & 0x3FF)) = value;
}
void scratchpad_store_16(uint32_t address, uint16_t value) {
  *(uint16_t*)(scratchpad + (address & 0x3FF)) = value;
}
void scratchpad_store_8(uint32_t address, uint8_t value) {
  *(uint8_t*)(scratchpad + (address & 0x3FF)) = value;
}
 memory_accessor_t scratchpad_accessor = {
  .load_32 = scratchpad_load_32,
  .load_16 = scratchpad_load_16,
  .load_8 = scratchpad_load_8,
  .store_32 = scratchpad_store_32,
  .store_16 = scratchpad_store_16,
  .store_8 = scratchpad_store_8,
};
