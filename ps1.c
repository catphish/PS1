#include <stdio.h>
#include <stdint.h>
#include "cpu.h"
#include "memory.h"

int main() {
  memory_load_bios();
  cpu_reset();

  while(1) {
    cpu_fetch_execute();
  }
  return(0);
}
