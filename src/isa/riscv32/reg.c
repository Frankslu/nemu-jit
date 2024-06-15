/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include "local-include/reg.h"

const char *regs[2][32] = {
  {
    "r0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
  },
  {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
  }
};

void isa_reg_display() {
  printf("    pc: " FMT_VADDR "\n", fmt_vaddr(cpu.pc));
  for (int i = 0; i < 32; i++) {
    printf("%02d %3s: " FMT_WORD "\n", i, reg_name(i), fmt_word(gpr(i)));
  }
}

word_t isa_reg_str2val(const char *s, bool *success) {
  *success = true;
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 32; j++) {
      if (streq(s, regs[i][j])) {
        return gpr(j);
      }
    }
  }
  if (streq(s, "pc")) {
    return cpu.pc;
  }
  *success = false;
  return 0;
}
