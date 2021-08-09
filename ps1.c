#include <stdio.h>
#include <stdint.h>
#include "cpu.h"
#include "memory.h"
#include "dma.h"
#include "rom.h"

int main() {
  rom_load_bios();
  cpu_reset();
  dma_reset();
  while(1) {
    cpu_fetch_execute();
  }
  return(0);
}
