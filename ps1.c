#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

char register_names[32][3] = {
                        "r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
                        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
                        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
                        "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
                      };

char cop_register_names[64][10] = {
    "Index",    "Random",   "EntryLo0", "EntryLo1",  // 00
    "Context",  "PageMask", "Wired",    "+Checkme",  // 04
    "BadVAddr", "Count",    "EntryHi",  "Compare",   // 08
    "Status",   "Cause",    "ExceptPC", "PRevID",    // 0c
    "Config",   "LLAddr",   "WatchLo",  "WatchHi",   // 10
    "XContext", "*RES*",    "*RES*",    "*RES*",     // 14
    "*RES*",    "*RES*",    "PErr",     "CacheErr",  // 18
    "TagLo",    "TagHi",    "ErrorEPC", "*RES*",     // 1c
  };

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
  // Multiplying by !!r causes zero to always be written to r0
  reg[r] = v * !!r;
}

void exception(uint32_t cause) {
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
    case 0x1F801814:; // GPU Status
      return 0x10000000;
    default:
      return(0);
  }
}

uint16_t load_16(uint32_t address) {
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
    default:
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
    default:
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
        // Ignoring write while cache is isolated
      } else
        *(uint32_t*)(main_ram + (address & 0x1FFFFF)) = value;
      break;
    default:
      break;
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
      if(cop_registers.sr & (1<<16)) {
        // Ignoring write while cache is isolated
      } else
        *(uint16_t*)(main_ram + (address & 0x1FFFFF)) = (uint16_t)value;
      break;
    default:
      break;
  }
}

void store_8(uint32_t address, uint8_t value) {
  switch(address) {
    case 0x00000000 ... 0x001FFFFF:;
    case 0x80000000 ... 0x801FFFFF:;
    case 0xA0000000 ... 0xA01FFFFF:;
      if(cop_registers.sr & (1<<16)) {
        // Ignoring write while cache is isolated
      } else
        *(uint8_t*)(main_ram + (address & 0x1FFFFF)) = (uint8_t)value;
      break;
    default:
      break;
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
  //printf("%08x ", current_pc);
  //printf("%08x: ", instruction);

  uint32_t location;

  switch(operation) {
    case 0x00:
      switch(operation_b) {
        case 0x00:
          if(instruction == 0) {
            //printf("nop    ");
          } else {
            //printf("sll    $%s(%08x), $%s(%08x), 0x%02x", register_names[rd], reg[rd], register_names[rt], reg[rt], imm5);
            set_reg(rd, reg[rt] << imm5);
          }
          break;
        case 0x02:
          //printf("srl    $%s(%08x), $%s(%08x), 0x%02x", register_names[rd], reg[rd], register_names[rt], reg[rt], imm5);
          set_reg(rd, reg[rt] >> imm5);
          break;
        case 0x03:
          //printf("sra    $%s(%08x), $%s(%08x), 0x%02x", register_names[rd], reg[rd], register_names[rt], reg[rt], imm5);
          set_reg(rd, (int32_t)reg[rt] >> imm5);
          break;
        case 0x04:
          //printf("sllv   ");
          set_reg(rd, reg[rt] << (reg[rs] & 0x1F));
          break;
        case 0x07:
          //printf("srav   ");
          set_reg(rd, (int32_t)reg[rt] >> (reg[rs] & 0x1F));
          break;
        case 0x08:
          //printf("jr     $%s(%08x)", register_names[rs], reg[rs]);
          next_pc = reg[rs];
          break;
        case 0x09:
          //printf("jalr   $%s(%08x), $%s(%08x)", register_names[rs], reg[rs], register_names[rd], reg[rd]);
          set_reg(rd, pc + 4);
          next_pc = reg[rs];
          break;
        case 0x0c:
          //printf("syscall");
          exception(8);
          break;
        case 0x10:
          //printf("mfhi   $%s(%08x), $hi(%08x)", register_names[rd], reg[rd], hi);
          set_reg(rd, hi);
          break;
        case 0x11:
          //printf("mthi   $hi(%08x), $%s(%08x)", hi, register_names[rs], reg[rs]);
          hi = reg[rs];
          break;
        case 0x12:
          //printf("mflo   $%s(%08x), $lo(%08x)", register_names[rd], reg[rd], lo);
          set_reg(rd, lo);
          break;
        case 0x13:
          //printf("mtlo   $lo(%08x), $%s(%08x)", lo, register_names[rs], reg[rs]);
          lo = reg[rs];
          break;
        case 0x1a:
          //printf("div    $%s(%08x), $%s(%08x)", register_names[rs], reg[rs], register_names[rt], reg[rt]);
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
          //printf("divu   ");
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
          //printf("add    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
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
          //printf("addu   $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
          set_reg(rd, reg[rs] + reg[rt]);
          break;
        case 0x23:
          //printf("subu   $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
          set_reg(rd, reg[rs] - reg[rt]);
          break;
        case 0x24:
          //printf("and    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
          set_reg(rd, reg[rs] & reg[rt]);
          break;
        case 0x25:
          //printf("or     $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
          set_reg(rd, reg[rs] | reg[rt]);
          break;
        case 0x26:
          //printf("xor    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
          set_reg(rd, reg[rs] ^ reg[rt]);
          break;
        case 0x27:
          //printf("nor    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
          set_reg(rd, ~(reg[rs] | reg[rt]));
          break;
        case 0x2A:
          //printf("slt    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
          set_reg(rd, (int32_t)reg[rs] < (int32_t)reg[rt]);
          break;
        case 0x2B:
          //printf("sltu   $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], reg[rd], register_names[rs], reg[rs], register_names[rt], reg[rt]);
          set_reg(rd, reg[rs] < reg[rt]);
          break;
        default:
          printf("Unknown operation 0x%08X OP:0x%02X/0x%02X RS:0x%02X RT:0x%02X RD:0x%02X\n", instruction, operation, operation_b, rs, rt, rd);
          exit(1);
      }
      break;
    case 0x01:;
      int bgez_instruction = (instruction >> 16) & 1;
      if(bgez_instruction) {
        //printf("bgez   $%s(%08x), 0x%08x", register_names[rs], reg[rs], pc + (int16_t)imm * 4);
      } else {
        //printf("bltz   $%s(%08x), 0x%08x", register_names[rs], reg[rs], pc + (int16_t)imm * 4);
      }
      int link_instruction = ((instruction >> 17) & 0xf) == 8;
      int result = (int32_t)reg[rs] < 0;
      result ^= bgez_instruction;
      if(link_instruction) set_reg(31, pc + 4);
      if(result) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x02:
      //printf("j      0x%08x", (pc & 0xF0000000) | ( imm26 << 2));
      next_pc = (pc & 0xF0000000) | ( imm26 << 2);
      break;
    case 0x03:
      //printf("jal    0x%08x", (pc & 0xF0000000) | ( imm26 << 2));
      set_reg(31, pc + 4);
      next_pc = (pc & 0xF0000000) | ( imm26 << 2);
      break;
    case 0x04:
      //printf("beq    $%s(%08x), $%s(%08x), 0x%08x", register_names[rs], reg[rs], register_names[rt], reg[rt], pc + (int16_t)imm * 4);
      if(reg[rs] == reg[rt]) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x05:
      //printf("bne    $%s(%08x), $%s(%08x), 0x%08x", register_names[rs], reg[rs], register_names[rt], reg[rt], pc + (int16_t)imm * 4);
      if(reg[rs] != reg[rt]) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x06:
      //printf("blez   $%s(%08x), 0x%08x", register_names[rs], reg[rs], pc + (int16_t)imm * 4);
      if((int32_t)reg[rs] <= 0) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x07:
      //printf("bgtz   $%s(%08x), 0x%08x", register_names[rs], reg[rs], pc + (int16_t)imm * 4);
      if((int32_t)reg[rs] > 0) next_pc = pc + (int16_t)imm * 4;
      break;
    case 0x08:
      //printf("addi   $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], reg[rt], register_names[rs], reg[rs], imm);
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
      //printf("addiu  $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], reg[rt], register_names[rs], reg[rs], imm);
      set_reg(rt, (int32_t)reg[rs] + (int16_t)imm);
      break;
    case 0x0A:
      //printf("slti   $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], reg[rt], register_names[rs], reg[rs], imm);
      set_reg(rt, (int32_t)reg[rs] < (int16_t)imm);
      break;
    case 0x0B:
      //printf("sltiu  $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], reg[rt], register_names[rs], reg[rs], imm);
      set_reg(rt, reg[rs] < (int16_t)imm);
      break;
    case 0x0C:
      //printf("andi   $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], reg[rt], register_names[rs], reg[rs], imm);
      set_reg(rt, reg[rs] & imm);
      break;
    case 0x0D:
      //printf("ori    $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], reg[rt], register_names[rs], reg[rs], imm);
      set_reg(rt, reg[rs] | imm);
      break;
    case 0x0F:
      //printf("lui    $%s(%08x), 0x%04x", register_names[rt], reg[rt], imm);
      set_reg(rt, imm << 16);
      break;
    case 0x10:
      switch(rs) {
        case 0x00:
          //printf("mfc0   $%s(%08x), $%s(%08x)", register_names[rt], reg[rt], cop_register_names[rd], cop_registers.reg[rd]);
          set_reg(rt, cop_registers.reg[rd]);
          break;
        case 0x04:
          //printf("mtc0   $%s(%08x), $%s(%08x)", cop_register_names[rd], cop_registers.reg[rd], register_names[rt], reg[rt]);
          cop_registers.reg[rd] = reg[rt];
          break;
        case 0x10:;
          //printf("rfe    ");
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
      location = reg[rs] + (int16_t)imm;
      //printf("lb     $%s(%08x), %i(%s)([%08x] = %02x)", register_names[rt], reg[rt], (int16_t)imm, register_names[rs], location, (int8_t)load_8(location));
      set_reg(rt, (int8_t)load_8(location));
      break;
    case 0x21:
      location = reg[rs] + (int16_t)imm;
      //printf("lh     $%s(%08x), %i(%s)([%08x] = %04x)", register_names[rt], reg[rt], (int16_t)imm, register_names[rs], location, (int16_t)load_16(location));
      set_reg(rt, (int16_t)load_16(location));
      break;
    case 0x23:;
      location = reg[rs] + (int16_t)imm;
      //printf("lw     $%s(%08x), %i(%s)([%08x] = %08x)", register_names[rt], reg[rt], (int16_t)imm, register_names[rs], location, load_32(location));
      set_reg(rt, load_32(location));
      break;
    case 0x24:
      location = reg[rs] + (int16_t)imm;
      //printf("lbu    $%s(%08x), %i(%s)([%08x] = %02x)", register_names[rt], reg[rt], (int16_t)imm, register_names[rs], location, load_8(location));
      set_reg(rt, load_8(location));
      break;
    case 0x25:
      location = reg[rs] + (int16_t)imm;
      //printf("lhu    $%s(%08x), %i(%s)([%08x] = %04x)", register_names[rt], reg[rt], (int16_t)imm, register_names[rs], location, load_16(location));
      set_reg(rt, (uint16_t)load_16(location));
      break;
    case 0x28:
      location = reg[rs] + (int16_t)imm;
      //printf("sb     $%s(%08x), %i(%s)([%08x] = %02x)", register_names[rt], reg[rt], (int16_t)imm, register_names[rs], location, load_8(location));
      store_8(location, (uint8_t)reg[rt]);
      break;
    case 0x29:
      location = reg[rs] + (int16_t)imm;
      //printf("sh     $%s(%08x), %i(%s)([%08x] = %04x)", register_names[rt], reg[rt], (int16_t)imm, register_names[rs], location, load_16(location));
      store_16(location, (uint16_t)reg[rt]);
      break;
    case 0x2B:;
      location = reg[rs] + (int16_t)imm;
      //printf("sw     $%s(%08x), %i(%s)([%08x] = %08x)", register_names[rt], reg[rt], (int16_t)imm, register_names[rs], location, load_32(location));
      store_32(location, reg[rt]);
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
    //printf("\n");
  }
  return(0);
}
