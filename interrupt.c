#include <stdint.h>
#include "memory.h"

 memory_accessor_t interrupt_accessor = {
  .load_32 = memory_dummy_load_32,
  .load_16 = memory_dummy_load_16,
  .load_8 = memory_dummy_load_8,
  .store_32 = memory_dummy_store_32,
  .store_16 = memory_dummy_store_16,
  .store_8 = memory_dummy_store_8,
};
