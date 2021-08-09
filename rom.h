#ifndef ROM_H
#define ROM_H

#include "memory.h"

extern memory_accessor_t rom_accessor;
void rom_load_bios();

#endif
