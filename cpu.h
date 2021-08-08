#include <stdint.h>

typedef union cop_registers {
 struct {
   uint32_t indx;
   uint32_t rand;
   uint32_t tlbl;
   uint32_t cpc;
   uint32_t ctxt;
   uint32_t bda;
   uint32_t pidmask;
   uint32_t dcic;
   uint32_t badv;
   uint32_t bdam;
   uint32_t tlbh;
   uint32_t bpcm;
   uint32_t sr;
   uint32_t cause;
   uint32_t epc;
   uint32_t prid;
   uint32_t erreg;
 };
 uint32_t reg[16];
} cop_registers_t;

extern cop_registers_t cpu_cop_registers;

void cpu_fetch_execute();
void cpu_exception(uint32_t cause);
void cpu_reset();