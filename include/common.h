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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <generated/autoconf.h>
#include <macro.h>

#ifdef CONFIG_TARGET_AM
#include <klib.h>
#else
#include <assert.h>
#include <stdlib.h>
#endif

#if CONFIG_MBASE + CONFIG_MSIZE > 0x100000000ul
#define PMEM64 1
#endif

typedef union {
  uint64_t word;
  struct {
    uint16_t a0;
    uint16_t a1;
    uint16_t a2;
    uint16_t a3;
  } fmt;
} _fmt_uint64;

typedef union {
  uint32_t word;
  struct {
    uint16_t a0;
    uint16_t a1;
  } fmt;
} _fmt_uint32;

#define FMT_UINT64 "0x%04" PRIx16 "_%04" PRIx16 "_%04" PRIx16 "_%04" PRIx16
#define fmt_uint64(x) ((_fmt_uint64)(x)).fmt.a3, ((_fmt_uint64)(x)).fmt.a2, \
  ((_fmt_uint64)(x)).fmt.a1, ((_fmt_uint64)(x)).fmt.a0

#define FMT_UINT32 "0x%04" PRIx16 "_%04" PRIx16
#define fmt_uint32(x) ((_fmt_uint32)(x)).fmt.a1, ((_fmt_uint32)(x)).fmt.a0

typedef MUXDEF(CONFIG_ISA64, uint64_t, uint32_t) word_t;
typedef MUXDEF(CONFIG_ISA64, int64_t, int32_t)  sword_t;
#define FMT_WORD MUXDEF(CONFIG_ISA64, FMT_UINT64, FMT_UINT32)
#define FMT_WORD_2 MUXDEF(CONFIG_ISA64, "0x%" PRIx64, "0x%" PRIx32)
#define fmt_word(x) MUXDEF(CONFIG_ISA64, fmt_uint64, fmt_uint32)(x)
#define WORD_MAX MUXDEF(CONFIG_ISA64, UINT64_MAX, UINT32_MAX)
#define SWORD_MAX MUXDEF(CONFIG_ISA64, INT64_MAX, INT32_MAX)

typedef word_t vaddr_t;
#define FMT_VADDR FMT_WORD
#define fmt_vaddr(x) fmt_word(x)
typedef MUXDEF(PMEM64, uint64_t, uint32_t) paddr_t;
#define FMT_PADDR MUXDEF(PMEM64, FMT_UINT64, FMT_UINT32)
#define fmt_paddr(x) MUXDEF(PMEM64, fmt_uint64, fmt_uint32)(x)
typedef uint16_t ioaddr_t;

#include <utils.h>

#endif
