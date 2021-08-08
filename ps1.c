#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

uint32_t pc, next_pc, current_pc, hi, lo;
uint32_t reg[32];
uint32_t sr;

uint8_t main_ram[1024*2048];
uint8_t rom[1024*512];

union cop_registers {
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
} cop_registers;

void load_bios() {
  int fd = open("BIOS/ps-22a.bin", O_RDONLY);
  int bytes = read(fd, rom, 524288);
  if(bytes != 524288) {
    printf("ROM READ FAILED!\n");
    exit(1);
  }
  close(fd);
}

void set_reg(uint8_t r, uint32_t v) {
  //printf("  Setting R%i to 0x%08X\n", r, v * !!r);
  reg[r] = v * !!r;
}

void exception(uint32_t cause) {
  printf("Exception! current_pc: 0x%08X pc: 0x%08X Cause: %u\n", current_pc, pc, cause);
  uint32_t handler;
  if(cop_registers.sr & (1<<22)) {
    handler = 0xbfc00180;
  } else {
    handler = 0x80000080;
  }

  uint32_t mode = cop_registers.sr << 2;
  mode &= 0x3F;
  cop_registers.sr &= ~0x3F;
  cop_registers.sr |= mode;

  cop_registers.cause = cause << 2;
  cop_registers.epc = current_pc;
  // This is a hack to guess whether we're in a branch delay slot
  if(pc != current_pc + 4) cop_registers.epc = current_pc - 4;

  pc = handler;
  next_pc = handler + 4;
}

uint32_t load_32(uint32_t address) {
  if(address % 4) {
    exception(4);
    return(0);
  }
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      return *(uint32_t*)(main_ram + (address & 0x1FFFFF));
    case 0x1FC00000 ... 0x1FC7FFFF:;
    case 0x9FC00000 ... 0x9FC7FFFF:;
    case 0xBFC00000 ... 0xBFC7FFFF:;
      return *(uint32_t*)(rom + (address & 0x7FFFF));
    case 0x1f801070 ... 0x1f801077:; // Interrupt control
    case 0x1f801080 ... 0x1f8010FF:; // DMA control
      return(0);
    default:
      printf("  Attempt to read invalid memory location 0x%08X\n", address);
      exit(1);
      return(0);
  }
}

uint8_t load_16(uint32_t address) {
  if(address % 2) {
    exception(4);
    return(0);
  }
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      return *(uint16_t*)(main_ram + (address & 0x1FFFFF));
      break;
    case 0x1FC00000 ... 0x1FC7FFFF:;
    case 0x9FC00000 ... 0x9FC7FFFF:;
    case 0xBFC00000 ... 0xBFC7FFFF:;
      return *(uint16_t*)(rom + (address & 0x7FFFF));
      break;
    case 0x1f801070 ... 0x1f801077:; // Interrupt control
    case 0x1f801c00 ... 0x1F801E7F:; // SPU
      return(0);
    default:
      printf("  Attempt to read invalid memory location 0x%08X\n", address);
      exit(1);
      return(0);
  }
}

uint8_t load_8(uint32_t address) {
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      return *(uint8_t*)(main_ram + (address & 0x1FFFFF));
      break;
    case 0x1FC00000 ... 0x1FC7FFFF:;
    case 0x9FC00000 ... 0x9FC7FFFF:;
    case 0xBFC00000 ... 0xBFC7FFFF:;
      return *(uint8_t*)(rom + (address & 0x7FFFF));
      break;
    case 0x1f801070 ... 0x1f801077:; // Interrupt control
    case 0x1f000000 ... 0x1f000000 + 512 * 1024 - 1:; // Expansion 1
      return(0);
    default:
      printf("  Attempt to read invalid memory location 0x%08X\n", address);
      exit(1);
      return(0);
  }
}


void store_32(uint32_t address, uint32_t value) {
  if(address % 4) {
    exception(5);
    return;
  }
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      if(cop_registers.sr & (1<<16)) {
        //printf("  Ignoring write to 0x%08X while cache is isolated\n", address);
      } else
        *(uint32_t*)(main_ram + (address & 0x1FFFFF)) = value;
      break;
    case 0x1f801000 ... 0x1f801023:; // Memory management
    case 0x1f801060:; // Memory management
    case 0xFFFE0130:; // Cache management
    case 0x1f801070 ... 0x1f801077:; // Interrupt control
    case 0x1f801080 ... 0x1f8010FF:; // DMA control
    case 0x1f801100 ... 0x1f801129:; // Timer control
      break;
    default:
      printf("  Attempt to write to invalid memory location 0x%08X\n", address);
      exit(1);
  }
}

void store_16(uint32_t address, uint16_t value) {
  if(address % 2) {
    exception(5);
    return;
  }
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      if(cop_registers.sr) {
        //printf("  Ignoring write to 0x%08X while cache is isolated\n", address);
      } else
        *(uint16_t*)(main_ram + (address & 0x1FFFFF)) = value;
      break;
    case 0x1f801c00 ... 0x1F801E7F:; // SPU
    case 0x1f801100 ... 0x1f801129:; // Timer control
    case 0x1f801070 ... 0x1f801077:; // Interrupt control
      break;
    default:
      printf("  Attempt to write to invalid memory location 0x%08X\n", address);
      exit(1);
  }
}

void store_8(uint32_t address, uint8_t value) {
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      if(cop_registers.sr) {
        //printf("  Ignoring write to 0x%08X while cache is isolated\n", address);
      } else
        *(uint8_t*)(main_ram + (address & 0x1FFFFF)) = value;
      break;
    case 0x1f802000 ... 0x1F802041:; // Expansion 2
    case 0x1f801070 ... 0x1f801077:; // Interrupt control
    case 0x1f801100 ... 0x1f801129:; // Timer control
      break;
    default:
      printf("  Attempt to write to invalid memory location 0x%08X\n", address);
      exit(1);
  }
}

void decode_and_execute(uint32_t instruction) {
  if(current_pc % 4) {
    exception(4);
    return;
  }
  uint8_t operation = instruction >> 26;
  uint8_t operation_b = instruction & 0x3F;
  uint8_t rs = (instruction >> 21) & 0x1F;
  uint8_t rt = (instruction >> 16) & 0x1F;
  uint8_t rd = (instruction >> 11) & 0x1F;
  uint32_t imm26 = (instruction & 0x3FFFFFF);
  uint16_t imm = (instruction & 0xFFFF);
  uint8_t imm5 = (instruction >> 6) & 0x1F;
  //printf("0x%08X: ", current_pc);
  //printf("Instruction: 0x%08X ", instruction);

  switch(operation) {
    case 0x00:
      switch(operation_b) {
        case 0x00:
          //printf("SLL: Shifting R%u(0x%08X) left by %u and storing in R%u\n", rt, reg[rt], imm5, rd);
          set_reg(rd, reg[rt] << imm5);
          break;
        case 0x02:
          //printf("SRL: Shifting R%u(0x%08X) right by %u and storing in R%u\n", rt, reg[rt], imm5, rd);
          set_reg(rd, reg[rt] >> imm5);
          break;
        case 0x03:
          //printf("SRA: Shifting R%u(%i) right by %u and storing in R%u\n", rt, reg[rt], imm5, rd);
          set_reg(rd, (int32_t)reg[rt] >> imm5);
          break;
        case 0x04:
          //printf("SLLV: Shifting R%u(0x%08X) left by R%u(%u) and storing in R%u\n", rt, reg[rt], rs, reg[rs], rd);
          set_reg(rd, reg[rt] << (reg[rs] & 0x1F));
          break;
        case 0x07:
          //printf("SRAV: Shifting R%u(%i) right by R%u(%u) and storing in R%u\n", rt, reg[rt], rs, reg[rs], rd);
          set_reg(rd, (int32_t)reg[rt] >> (reg[rs] & 0x1F));
          break;
        case 0x08:
          //printf("JR: Jumping to address in R%u (0x%08X)\n", rs, reg[rs]);
          next_pc = reg[rs];
          break;
        case 0x09:
          //printf("JALR: Jumping to address in R%u (0x%08X) and storing return address in R%u\n", rs, reg[rs], rd);
          set_reg(rd, pc + 4);
          next_pc = reg[rs];
          break;
        case 0x0c:
          //printf("SYSCALL\n");
          exception(8);
          break;
        case 0x10:
          //printf("MFHI: Moving HI to R%u\n", rd);
          set_reg(rd, hi);
          break;
        case 0x11:
          //printf("MTHI: Moving R%u to HI\n", rs);
          hi = reg[rs];
          break;
        case 0x12:
          //printf("MFLO: Moving LO to R%u\n", rd);
          set_reg(rd, lo);
          break;
        case 0x13:
          //printf("MTLO: Moving R%u to LO\n", rs);
          lo = reg[rs];
          break;
        case 0x1a:
          //printf("DIV: Dividing R%u(%i) by R%u(%i) and storing value in HI/LO\n", rs, (int32_t)reg[rs], rt, (int32_t)reg[rt]);
          if(reg[rt]) {
            lo = (int32_t)reg[rs] / (int32_t)reg[rt];
            hi = (int32_t)reg[rs] % (int32_t)reg[rt];
          } else {
            // Divide by zero
            hi = reg[rs];
            if((int32_t)rs >= 0) lo = -1; else lo = 1;
          }
          break;
        case 0x1b:
          //printf("DIVU: Dividing R%u(%u) by R%u(%u) and storing value in HI/LO\n", rs, reg[rs], rt, reg[rt]);
          if(reg[rt]) {
            lo = reg[rs] / reg[rt];
            hi = reg[rs] % reg[rt];
          } else {
            // Divide by zero
            hi = reg[rs];
            lo = -1;
          }
          break;
        case 0x20:
          //printf("ADD: Adding R%u(0x%08X) to R%u(0x%08X) and storing into R%u\n", rs, reg[rs], rt, reg[rt], rd);
          if ((int32_t)reg[rs] >= 0) {
              if ((int32_t)reg[rt] > (INT32_MAX - (int32_t)reg[rs])) {
                  //printf("  OVERFLOW!");
                  exception(0xC);
                  break;
              }
          } else {
              if ((int32_t)reg[rt] < (INT32_MIN - (int32_t)reg[rs])) {
                  //printf("  OVERFLOW!");
                  exception(0xC);
                  break;
              }
          }
          set_reg(rd, (int32_t)reg[rs] + (int32_t)reg[rt]);
          break;
        case 0x21:
          //printf("ADDU: Setting R%u to R%u(0x%08X) + R%u(0x%08X)\n", rd, rs, reg[rs], rt, reg[rt]);
          set_reg(rd, reg[rs] + reg[rt]);
          break;
        case 0x23:
          //printf("SUBU: Setting R%u to R%u(0x%08X) - R%u(0x%08X)\n", rd, rs, reg[rs], rt, reg[rt]);
          set_reg(rd, reg[rs] - reg[rt]);
          break;
        case 0x24:
          //printf("AND: Setting R%u to R%u(0x%08X) & R%u(0x%08X)\n", rd, rs, reg[rs], rt, reg[rt]);
          set_reg(rd, reg[rs] & reg[rt]);
          break;
        case 0x25:
          //printf("OR: Setting R%u to R%u(0x%08X) | R%u(0x%08X)\n", rd, rs, reg[rs], rt, reg[rt]);
          set_reg(rd, reg[rs] | reg[rt]);
          break;
        case 0x27:
          //printf("NOR: Setting R%u to inverse of R%u(0x%08X) | R%u(0x%08X)\n", rd, rs, reg[rs], rt, reg[rt]);
          set_reg(rd, ~(reg[rs] | reg[rt]));
          break;
        case 0x2A:
          //printf("SLT: Setting R%u to R%u(%i) < R%u(%i)\n", rd, rs, (int32_t)reg[rs], rt, (int32_t)reg[rt]);
          set_reg(rd, (int32_t)reg[rs] < (int32_t)reg[rt]);
          break;
        case 0x2B:
          //printf("SLTU: Setting R%u to R%u(0x%08X) < R%u(0x%08X)\n", rd, rs, reg[rs], rt, reg[rt]);
          set_reg(rd, reg[rs] < reg[rt]);
          break;
        default:
          printf("Unknown operation 0x%08X OP:0x%02X/0x%02X RS:0x%02X RT:0x%02X RD:0x%02X\n", instruction, operation, operation_b, rs, rt, rd);
          exit(1);
      }
      break;
    case 0x01:;
      //printf("BLTZ / BGEZ\n");
      int bgez_instruction = (instruction >> 16) & 1;
      int link_instruction = ((instruction >> 17) & 0xf) == 8;
      int result = (int32_t)reg[rs] < 0;
      result ^= bgez_instruction;
      if(link_instruction) set_reg(31, pc + 4);
      if(result) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x02:
      //printf("J: Jumping to 0x%08X\n", (pc & 0xF0000000) | ( imm26 << 2));
      next_pc = (pc & 0xF0000000) | ( imm26 << 2);
      break;
    case 0x03:
      //printf("JAL: Jumping to 0x%08X and storing return address in R31\n", (pc & 0xF0000000) | ( imm26 << 2));
      set_reg(31, pc + 4);
      next_pc = (pc & 0xF0000000) | ( imm26 << 2);
      break;
    case 0x04:
      //printf("BEQ: Branching by offset %i if R%u(0x%08X) == R%u(0x%08X)\n", (int16_t)imm * 4, rs, reg[rs], rt, reg[rt]);
      if(reg[rs] == reg[rt]) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x05:
      //printf("BNE: Branching by offset %i if R%u(0x%08X) != R%u(0x%08X)\n", (int16_t)imm * 4, rs, reg[rs], rt, reg[rt]);
      if(reg[rs] != reg[rt]) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x06:
      //printf("BLEZ: Branching by offset %i if R%u(%i) <= 0\n", (int16_t)imm * 4, rs, (int32_t)reg[rs]);
      if((int32_t)reg[rs] <= 0) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x07:
      //printf("BGTZ: Branching by offset %i if R%u(%i) > 0\n", (int16_t)imm * 4, rs, (int32_t)reg[rs]);
      if((int32_t)reg[rs] > 0) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x08:
      //printf("ADDI: Adding %i to R%u(%i) and storing into R%u\n", (int16_t)imm, rs, (int32_t)reg[rs], rt);
      if ((int32_t)reg[rs] >= 0) {
          if ((int16_t)imm > (INT32_MAX - (int32_t)reg[rs])) {
            //printf("  OVERFLOW!");
            exception(0xC);
            break;
          }
      } else {
          if ((int16_t)imm < (INT32_MIN - (int32_t)reg[rs])) {
            //printf("  OVERFLOW!");
            exception(0xC);
            break;
          }
      }
      set_reg(rt, (int32_t)reg[rs] + (int16_t)imm);
      break;
    case 0x09:
      //printf("ADDIU: Adding %i to R%u(%i) and storing into R%u\n", (int16_t)imm, rs, (int32_t)reg[rs], rt);
      set_reg(rt, (int32_t)reg[rs] + (int16_t)imm);
      break;
    case 0x0A:
      //printf("SLTI: Setting R%u to R%u(%i) < %i\n", rt, rs, (int32_t)reg[rs], (int16_t)imm);
      set_reg(rt, (int32_t)reg[rs] < (int16_t)imm);
      break;
    case 0x0B:
      //printf("SLTIU: Setting R%u to R%u(%u) < %u\n", rt, rs, reg[rs], (int16_t)imm);
      set_reg(rt, reg[rs] < (int16_t)imm);
      break;
    case 0x0C:
      //printf("ANDI: AND 0x%04X with R%u(0x%08X) and storing into R%u\n", imm, rs, reg[rs], rt);
      set_reg(rt, reg[rs] & imm);
      break;
    case 0x0D:
      //printf("ORI: OR 0x%04X with R%u(0x%08X) and storing into R%u\n", imm, rs, reg[rs], rt);
      set_reg(rt, reg[rs] | imm);
      break;
    case 0x0F:
      //printf("LUI: Putting 0x%08X into R%u\n", imm << 16, rt);
      set_reg(rt, imm << 16);
      break;
    case 0x10:
      switch(rs) {
        case 0x00:
          //printf("MFC0: Moving R%u from coprocessor R%u(0x%08X)\n", rt, rd, cop_registers.reg[rd]);
          set_reg(rt, cop_registers.reg[rd]);
          break;
        case 0x04:
          //printf("MTC0: Moving R%u(0x%08X) to coprocessor R%u\n", rt, reg[rt], rd);
          cop_registers.reg[rd] = reg[rt];
          break;
        case 0x10:;
          //printf("RFE\n");
          if(operation_b != 0x10) {
            printf("Unknown coprocessor operation 0x%08X RS:0x%02X &0x3F:%02X\n", instruction, rs, operation_b);
            exit(1);
          }
          uint32_t mode = cop_registers.sr & 0x3F;
          cop_registers.sr &= ~0x3F;
          cop_registers.sr |= (mode >> 2);
          break;
        default:
          printf("Unknown operation 0x%08X OP:0x%02X RS:0x%02X RT:0x%02X RD:0x%02X\n", instruction, operation, rs, rt, rd);
          exit(1);
      }
      break;
    case 0x20:
      //printf("LB: Loading signed byte from R%u(0x%08X) + %i into R%u\n", rs, reg[rs], (int16_t)imm, rt);
      set_reg(rt, (int8_t)load_8(reg[rs] + (int16_t)imm));
      break;
    case 0x21:
      //printf("LH: Loading signed halfword from R%u(0x%08X) + %i into R%u\n", rs, reg[rs], (int16_t)imm, rt);
      set_reg(rt, (int16_t)load_16(reg[rs] + (int16_t)imm));
      break;
    case 0x23:
      //printf("LW: Loading word from R%u(0x%08X) + %i into R%u\n", rs, reg[rs], (int16_t)imm, rt);
      set_reg(rt, load_32(reg[rs] + (int16_t)imm));
      break;
    case 0x24:
      //printf("LBU: Loading unsigned byte from R%u(0x%08X) + %i into R%u\n", rs, reg[rs], (int16_t)imm, rt);
      set_reg(rt, (uint8_t)load_8(reg[rs] + (int16_t)imm));
      break;
    case 0x25:
      //printf("LHU: Loading unsigned halfword from R%u(0x%08X) + %i into R%u\n", rs, reg[rs], (int16_t)imm, rt);
      set_reg(rt, (uint16_t)load_16(reg[rs] + (int16_t)imm));
      break;
    case 0x28:
      //printf("SB: Storing byte of R%u(0x%08X) to R%u(0x%08X) + %i\n", rt, (uint8_t)reg[rt], rs, reg[rs], (int16_t)imm);
      store_8(reg[rs] + (int16_t)imm, (uint8_t)reg[rt]);
      break;
    case 0x29:
      //printf("SH: Storing halfword of R%u(0x%08X) to R%u(0x%08X) + %i\n", rt, (uint16_t)reg[rt], rs, reg[rs], (int16_t)imm);
      store_16(reg[rs] + (int16_t)imm, (uint16_t)reg[rt]);
      break;
    case 0x2B:
      //printf("SW: Storing word R%u(0x%08X) to R%u(0x%08X) + %i\n", rt, reg[rt], rs, reg[rs], (int16_t)imm);
      store_32(reg[rs] + (int16_t)imm, reg[rt]);
      break;
    default:
      printf("Unknown operation 0x%08X OP:0x%02X RS:0x%02X RT:0x%02X RD:0x%02X IMM:0x%04X\n", instruction, operation, rs, rt, rd, imm);
      exit(1);
  }
}

void reset() {
  pc = 0xbfc00000;
  next_pc = pc + 4;
  sr = 0;
  reg[0] = 0;
}

int main() {
  uint32_t instruction;

  load_bios();
  reset();

  while(1) {
    instruction = load_32(pc);
    current_pc = pc;
    pc = next_pc;
    next_pc = pc + 4;
    decode_and_execute(instruction);
  }
  return(0);
}
