#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "memory.h"

const char register_names[32][3] = {
  "r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
  "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
};

const char cop_register_names[64][9] = {
  "Index",    "Random",   "EntryLo0", "EntryLo1",
  "Context",  "PageMask", "Wired",    "+Checkme",
  "BadVAddr", "Count",    "EntryHi",  "Compare",
  "Status",   "Cause",    "ExceptPC", "PRevID",
  "Config",   "LLAddr",   "WatchLo",  "WatchHi",
  "XContext", "*RES*",    "*RES*",    "*RES*",
  "*RES*",    "*RES*",    "PErr",     "CacheErr",
  "TagLo",    "TagHi",    "ErrorEPC", "*RES*"
};

cpu_t cpu;

void cpu_set_reg(uint8_t r, uint32_t v) {
  // Multiplying by !!r causes zero to always be written to r0
  cpu.reg[r] = v * !!r;
}

void cpu_exception(uint32_t cause) {
  uint32_t handler;
  if(cpu.cop0_registers.sr & (1<<22)) {
    handler = 0xbfc00180;
  } else {
    handler = 0x80000080;
  }

  uint32_t mode = cpu.cop0_registers.sr << 2;
  mode &= 0x3F;
  cpu.cop0_registers.sr &= ~0x3F;
  cpu.cop0_registers.sr |= mode;

  cpu.cop0_registers.cause = cause << 2;
  cpu.cop0_registers.epc = cpu.current_pc;
  // This is a hack to guess whether we're in a branch delay slot
  if(cpu.pc != cpu.current_pc + 4) cpu.cop0_registers.epc = cpu.current_pc - 4;

  cpu.pc = handler;
  cpu.next_pc = handler + 4;
}

void decode_and_execute(uint32_t instruction) {
  if(cpu.current_pc % 4) {
    cpu_exception(4);
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
  //printf("%08x ", cpu.current_pc);
  //printf("%08x: ", instruction);

  uint32_t location;
  uint32_t aligned_word;

  switch(operation) {
    case 0x00:
      switch(operation_b) {
        case 0x00:
          if(instruction == 0) {
            //printf("nop    ");
          } else {
            //printf("sll    $%s(%08x), $%s(%08x), 0x%02x", register_names[rd], cpu.reg[rd], register_names[rt], cpu.reg[rt], imm5);
            cpu_set_reg(rd, cpu.reg[rt] << imm5);
          }
          break;
        case 0x02:
          //printf("srl    $%s(%08x), $%s(%08x), 0x%02x", register_names[rd], cpu.reg[rd], register_names[rt], cpu.reg[rt], imm5);
          cpu_set_reg(rd, cpu.reg[rt] >> imm5);
          break;
        case 0x03:
          //printf("sra    $%s(%08x), $%s(%08x), 0x%02x", register_names[rd], cpu.reg[rd], register_names[rt], cpu.reg[rt], imm5);
          cpu_set_reg(rd, (int32_t)cpu.reg[rt] >> imm5);
          break;
        case 0x04:
          //printf("sllv   ");
          cpu_set_reg(rd, cpu.reg[rt] << (cpu.reg[rs] & 0x1F));
          break;
        case 0x06:
          //printf("srlv   ");
          cpu_set_reg(rd, cpu.reg[rt] >> (cpu.reg[rs] & 0x1F));
          break;
        case 0x07:
          //printf("srav   ");
          cpu_set_reg(rd, (int32_t)cpu.reg[rt] >> (cpu.reg[rs] & 0x1F));
          break;
        case 0x08:
          //printf("jr     $%s(%08x)", register_names[rs], cpu.reg[rs]);
          cpu.next_pc = cpu.reg[rs];
          break;
        case 0x09:
          //printf("jalr   $%s(%08x), $%s(%08x)", register_names[rs], cpu.reg[rs], register_names[rd], cpu.reg[rd]);
          cpu_set_reg(rd, cpu.pc + 4);
          cpu.next_pc = cpu.reg[rs];
          break;
        case 0x0c:
          //printf("syscall");
          cpu_exception(8);
          break;
        case 0x10:
          //printf("mfhi   $%s(%08x), $hi(%08x)", register_names[rd], cpu.reg[rd], cpu.hi);
          cpu_set_reg(rd, cpu.hi);
          break;
        case 0x11:
          //printf("mthi   $hi(%08x), $%s(%08x)", cpu.hi, register_names[rs], cpu.reg[rs]);
          cpu.hi = cpu.reg[rs];
          break;
        case 0x12:
          //printf("mflo   $%s(%08x), $lo(%08x)", register_names[rd], cpu.reg[rd], cpu.lo);
          cpu_set_reg(rd, cpu.lo);
          break;
        case 0x13:
          //printf("mtlo   $lo(%08x), $%s(%08x)", cpu.lo, register_names[rs], cpu.reg[rs]);
          cpu.lo = cpu.reg[rs];
          break;
        case 0x1a:
          //printf("div    $%s(%08x), $%s(%08x)", register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          if(cpu.reg[rt]) {
            cpu.lo = (int32_t)cpu.reg[rs] / (int32_t)cpu.reg[rt];
            cpu.hi = (int32_t)cpu.reg[rs] % (int32_t)cpu.reg[rt];
          } else {
            // Divide by zero
            cpu.hi = cpu.reg[rs];
            if((int32_t)rs >= 0)
              cpu.lo = -1;
            else
              cpu.lo = 1;
          }
          break;
        case 0x19:;
          //printf("multu");
          uint64_t result = (uint64_t)cpu.reg[rs] * (uint64_t)cpu.reg[rt];
          cpu.hi = result >> 32;
          cpu.lo = result;
          break;
        case 0x1b:
          //printf("divu   ");
          if(cpu.reg[rt]) {
            cpu.lo = cpu.reg[rs] / cpu.reg[rt];
            cpu.hi = cpu.reg[rs] % cpu.reg[rt];
          } else {
            // Divide by zero
            cpu.hi = cpu.reg[rs];
            cpu.lo = -1;
          }
          break;
        case 0x20:
          //printf("add    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          if ((int32_t)cpu.reg[rs] >= 0) {
              if ((int32_t)cpu.reg[rt] > (INT32_MAX - (int32_t)cpu.reg[rs])) {
                  //printf("  OVERFLOW!");
                  cpu_exception(0xC);
                  break;
              }
          } else {
              if ((int32_t)cpu.reg[rt] < (INT32_MIN - (int32_t)cpu.reg[rs])) {
                  //printf("  OVERFLOW!");
                  cpu_exception(0xC);
                  break;
              }
          }
          cpu_set_reg(rd, (int32_t)cpu.reg[rs] + (int32_t)cpu.reg[rt]);
          break;
        case 0x21:
          //printf("addu   $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          cpu_set_reg(rd, cpu.reg[rs] + cpu.reg[rt]);
          break;
        case 0x23:
          //printf("subu   $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          cpu_set_reg(rd, cpu.reg[rs] - cpu.reg[rt]);
          break;
        case 0x24:
          //printf("and    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          cpu_set_reg(rd, cpu.reg[rs] & cpu.reg[rt]);
          break;
        case 0x25:
          //printf("or     $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          cpu_set_reg(rd, cpu.reg[rs] | cpu.reg[rt]);
          break;
        case 0x26:
          //printf("xor    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          cpu_set_reg(rd, cpu.reg[rs] ^ cpu.reg[rt]);
          break;
        case 0x27:
          //printf("nor    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          cpu_set_reg(rd, ~(cpu.reg[rs] | cpu.reg[rt]));
          break;
        case 0x2A:
          //printf("slt    $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          cpu_set_reg(rd, (int32_t)cpu.reg[rs] < (int32_t)cpu.reg[rt]);
          break;
        case 0x2B:
          //printf("sltu   $%s(%08x), $%s(%08x), $%s(%08x)", register_names[rd], cpu.reg[rd], register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt]);
          cpu_set_reg(rd, cpu.reg[rs] < cpu.reg[rt]);
          break;
        default:
          printf("Unknown operation 0x%08X OP:0x%02X/0x%02X RS:0x%02X RT:0x%02X RD:0x%02X\n", instruction, operation, operation_b, rs, rt, rd);
          exit(1);
      }
      break;
    case 0x01:;
      int bgez_instruction = (instruction >> 16) & 1;
      if(bgez_instruction) {
        //printf("bgez   $%s(%08x), 0x%08x", register_names[rs], cpu.reg[rs], cpu.pc + (int16_t)imm * 4);
      } else {
        //printf("bltz   $%s(%08x), 0x%08x", register_names[rs], cpu.reg[rs], cpu.pc + (int16_t)imm * 4);
      }
      int link_instruction = ((instruction >> 17) & 0xf) == 8;
      int result = (int32_t)cpu.reg[rs] < 0;
      result ^= bgez_instruction;
      if(link_instruction) cpu_set_reg(31, cpu.pc + 4);
      if(result) cpu.next_pc = cpu.pc + (int16_t)imm * 4;
      break;
    case 0x02:
      //printf("j      0x%08x", (cpu.pc & 0xF0000000) | ( imm26 << 2));
      cpu.next_pc = (cpu.pc & 0xF0000000) | ( imm26 << 2);
      break;
    case 0x03:
      //printf("jal    0x%08x", (cpu.pc & 0xF0000000) | ( imm26 << 2));
      cpu_set_reg(31, cpu.pc + 4);
      cpu.next_pc = (cpu.pc & 0xF0000000) | ( imm26 << 2);
      break;
    case 0x04:
      //printf("beq    $%s(%08x), $%s(%08x), 0x%08x", register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt], cpu.pc + (int16_t)imm * 4);
      if(cpu.reg[rs] == cpu.reg[rt]) cpu.next_pc = cpu.pc + (int16_t)imm * 4;
      break;
    case 0x05:
      //printf("bne    $%s(%08x), $%s(%08x), 0x%08x", register_names[rs], cpu.reg[rs], register_names[rt], cpu.reg[rt], cpu.pc + (int16_t)imm * 4);
      if(cpu.reg[rs] != cpu.reg[rt]) cpu.next_pc = cpu.pc + (int16_t)imm * 4;
      break;
    case 0x06:
      //printf("blez   $%s(%08x), 0x%08x", register_names[rs], cpu.reg[rs], cpu.pc + (int16_t)imm * 4);
      if((int32_t)cpu.reg[rs] <= 0) cpu.next_pc = cpu.pc + (int16_t)imm * 4;
      break;
    case 0x07:
      //printf("bgtz   $%s(%08x), 0x%08x", register_names[rs], cpu.reg[rs], cpu.pc + (int16_t)imm * 4);
      if((int32_t)cpu.reg[rs] > 0) cpu.next_pc = cpu.pc + (int16_t)imm * 4;
      break;
    case 0x08:
      //printf("addi   $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], cpu.reg[rt], register_names[rs], cpu.reg[rs], imm);
      if ((int32_t)cpu.reg[rs] >= 0) {
          if ((int16_t)imm > (INT32_MAX - (int32_t)cpu.reg[rs])) {
            //printf("  OVERFLOW!");
            cpu_exception(0xC);
            break;
          }
      } else {
          if ((int16_t)imm < (INT32_MIN - (int32_t)cpu.reg[rs])) {
            //printf("  OVERFLOW!");
            cpu_exception(0xC);
            break;
          }
      }
      cpu_set_reg(rt, (int32_t)cpu.reg[rs] + (int16_t)imm);
      break;
    case 0x09:
      //printf("addiu  $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], cpu.reg[rt], register_names[rs], cpu.reg[rs], imm);
      cpu_set_reg(rt, (int32_t)cpu.reg[rs] + (int16_t)imm);
      break;
    case 0x0A:
      //printf("slti   $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], cpu.reg[rt], register_names[rs], cpu.reg[rs], imm);
      cpu_set_reg(rt, (int32_t)cpu.reg[rs] < (int16_t)imm);
      break;
    case 0x0B:
      //printf("sltiu  $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], cpu.reg[rt], register_names[rs], cpu.reg[rs], imm);
      cpu_set_reg(rt, cpu.reg[rs] < (int16_t)imm);
      break;
    case 0x0C:
      //printf("andi   $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], cpu.reg[rt], register_names[rs], cpu.reg[rs], imm);
      cpu_set_reg(rt, cpu.reg[rs] & imm);
      break;
    case 0x0D:
      //printf("ori    $%s(%08x), $%s(%08x), 0x%04x", register_names[rt], cpu.reg[rt], register_names[rs], cpu.reg[rs], imm);
      cpu_set_reg(rt, cpu.reg[rs] | imm);
      break;
    case 0x0F:
      //printf("lui    $%s(%08x), 0x%04x", register_names[rt], cpu.reg[rt], imm);
      cpu_set_reg(rt, imm << 16);
      break;
    case 0x10:
      switch(rs) {
        case 0x00:
          //printf("mfc0   $%s(%08x), $%s(%08x)", register_names[rt], cpu.reg[rt], cop_register_names[rd], cpu.cop0_reg[rd]);
          cpu_set_reg(rt, cpu.cop0_reg[rd]);
          break;
        case 0x04:
          //printf("mtc0   $%s(%08x), $%s(%08x)", cop_register_names[rd], cpu.cop0_reg[rd], register_names[rt], cpu.reg[rt]);
          cpu.cop0_reg[rd] = cpu.reg[rt];
          break;
        case 0x10:;
          //printf("rfe    ");
          if(operation_b != 0x10) {
            printf("Unknown coprocessor operation 0x%08X RS:0x%02X &0x3F:%02X\n", instruction, rs, operation_b);
            exit(1);
          }
          uint32_t mode = cpu.cop0_registers.sr & 0x3F;
          cpu.cop0_registers.sr &= ~0x3F;
          cpu.cop0_registers.sr |= (mode >> 2);
          break;
        default:
          printf("Unknown operation 0x%08X OP:0x%02X RS:0x%02X RT:0x%02X RD:0x%02X\n", instruction, operation, rs, rt, rd);
          exit(1);
      }
      break;
    case 0x20:
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("lb     $%s(%08x), %i(%s)([%08x] = %02x)", register_names[rt], cpu.reg[rt], (int16_t)imm, register_names[rs], location, (int8_t)memory_load_8(location));
      cpu_set_reg(rt, (int8_t)memory_load_8(location));
      break;
    case 0x21:
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("lh     $%s(%08x), %i(%s)([%08x] = %04x)", register_names[rt], cpu.reg[rt], (int16_t)imm, register_names[rs], location, (int16_t)memory_load_16(location));
      cpu_set_reg(rt, (int16_t)memory_load_16(location));
      break;
    case 0x22:
      //printf("lwl\n");
      location = cpu.reg[rs] + (int16_t)imm;
      aligned_word = memory_load_32(location & ~3);
      switch(location & ~3) {
        case 0: cpu.reg[rt] = (cpu.reg[rt] & 0x00ffffff) | (aligned_word << 24); break;
        case 1: cpu.reg[rt] = (cpu.reg[rt] & 0x0000ffff) | (aligned_word << 16); break;
        case 2: cpu.reg[rt] = (cpu.reg[rt] & 0x000000ff) | (aligned_word << 8); break;
        case 3: cpu.reg[rt] = (cpu.reg[rt] & 0x00000000) | (aligned_word << 0); break;
      }
      break;
    case 0x23:;
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("lw     $%s(%08x), %i(%s)([%08x] = %08x)", register_names[rt], cpu.reg[rt], (int16_t)imm, register_names[rs], location, memory_load_32(location));
      cpu_set_reg(rt, memory_load_32(location));
      break;
    case 0x24:
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("lbu    $%s(%08x), %i(%s)([%08x] = %02x)", register_names[rt], cpu.reg[rt], (int16_t)imm, register_names[rs], location, memory_load_8(location));
      cpu_set_reg(rt, memory_load_8(location));
      break;
    case 0x25:
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("lhu    $%s(%08x), %i(%s)([%08x] = %04x)", register_names[rt], cpu.reg[rt], (int16_t)imm, register_names[rs], location, memory_load_16(location));
      cpu_set_reg(rt, (uint16_t)memory_load_16(location));
      break;
    case 0x26:
      //printf("lwr\n");
      location = cpu.reg[rs] + (int16_t)imm;
      aligned_word = memory_load_32(location & ~3);
      switch(location & ~3) {
        case 0: cpu.reg[rt] = (cpu.reg[rt] & 0x00000000) | (aligned_word >> 0); break;
        case 1: cpu.reg[rt] = (cpu.reg[rt] & 0xff000000) | (aligned_word >> 8); break;
        case 2: cpu.reg[rt] = (cpu.reg[rt] & 0xffff0000) | (aligned_word >> 16); break;
        case 3: cpu.reg[rt] = (cpu.reg[rt] & 0xffffff00) | (aligned_word >> 24); break;
      }
      break;
    case 0x28:
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("sb     $%s(%08x), %i(%s)([%08x] = %02x)", register_names[rt], cpu.reg[rt], (int16_t)imm, register_names[rs], location, memory_load_8(location));
      memory_store_8(location, (uint8_t)cpu.reg[rt]);
      break;
    case 0x29:
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("sh     $%s(%08x), %i(%s)([%08x] = %04x)", register_names[rt], cpu.reg[rt], (int16_t)imm, register_names[rs], location, memory_load_16(location));
      memory_store_16(location, (uint16_t)cpu.reg[rt]);
      break;
    case 0x2a:
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("swl");
      aligned_word = memory_load_32(location & ~3);
      switch(location & ~3) {
        case 0: aligned_word = (aligned_word & 0xffffff00) | (cpu.reg[rt] >> 24); break;
        case 1: aligned_word = (aligned_word & 0xffff0000) | (cpu.reg[rt] >> 16); break;
        case 2: aligned_word = (aligned_word & 0xff000000) | (cpu.reg[rt] >> 8); break;
        case 3: aligned_word = (aligned_word & 0x00000000) | (cpu.reg[rt] >> 0); break;
      }
      memory_store_32(location & ~3, aligned_word);
      break;
    case 0x2B:;
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("sw     $%s(%08x), %i(%s)([%08x] = %08x)", register_names[rt], cpu.reg[rt], (int16_t)imm, register_names[rs], location, memory_load_32(location));
      memory_store_32(location, cpu.reg[rt]);
      break;
    case 0x2e:
      location = cpu.reg[rs] + (int16_t)imm;
      //printf("swr");
      aligned_word = memory_load_32(location & ~3);
      switch(location & ~3) {
        case 0: aligned_word = (aligned_word & 0x00000000) | (cpu.reg[rt] << 0); break;
        case 1: aligned_word = (aligned_word & 0x000000ff) | (cpu.reg[rt] << 8); break;
        case 2: aligned_word = (aligned_word & 0x0000ffff) | (cpu.reg[rt] << 16); break;
        case 3: aligned_word = (aligned_word & 0x00ffffff) | (cpu.reg[rt] << 24); break;
      }
      memory_store_32(location & ~3, aligned_word);
      break;
    default:
      printf("Unknown operation 0x%08X OP:0x%02X RS:0x%02X RT:0x%02X RD:0x%02X IMM:0x%04X\n", instruction, operation, rs, rt, rd, imm);
      exit(1);
  }
  //printf("\n");
}

void cpu_reset() {
  cpu.pc = 0xbfc00000;
  cpu.next_pc = cpu.pc + 4;
  cpu.sr = 0;
  cpu.reg[0] = 0;
}

uint32_t fetch_next_instruction() {
  uint32_t instruction = memory_load_32(cpu.pc);
  cpu.current_pc = cpu.pc;
  cpu.pc = cpu.next_pc;
  cpu.next_pc = cpu.pc + 4;
  return instruction;
}

void cpu_fetch_execute() {
  decode_and_execute(fetch_next_instruction());
}
