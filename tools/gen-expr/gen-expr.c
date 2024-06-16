/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#include <common.h>

typedef char Opcode[8];

// buf written to C program
static char c_buf[65536] = {};
// buf written to input.txt
static char expr_buf[65536] = {};
static char code_buf[65536 + 512] = {}; // a little larger than `buf`
int c_buf_pos = 0;
int expr_buf_pos = 0;
int token_num = 0;
char num_str[64] = {};
char fuc_num = 0;

#ifdef CONFIG_ISA64
static char *code_format1 =
"#include <stdio.h>\n"
"#include <stdint.h>\n"
"int main() { \n"
"#pragma GCC diagnostic push\n"
"#pragma GCC diagnostic ignored \"-Wparentheses\"\n"
"  uint64_t result = %s;\n"
"  printf(\"%%" PRIx64 "\", result); \n"
"  return 0; \n"
"#pragma GCC diagnostic pop\n"
"}";
#else
static char *code_format1 =
"#include <stdio.h>\n"
"#include <stdint.h>\n"
"int main() { \n"
"#pragma GCC diagnostic push\n"
"#pragma GCC diagnostic ignored \"-Wparentheses\"\n"
"  uint32_t result = %s;\n"
"  printf(\"%%"PRIx32"\", result); \n"
"  return 0; \n"
"#pragma GCC diagnostic pop\n"
"}";
#endif

#define WORDdfmt MUXDEF(CONFIG_ISA64, PRIu64, PRIu32)
#define WORDxfmt MUXDEF(CONFIG_ISA64, PRIx64, PRIx32)
#define suffix MUXDEF(CONFIG_ISA64, "LU", "U")

static void gen_num() {
#ifdef CONFIG_ISA64
  word_t num = ((uint64_t)rand()) << 32 | ((uint64_t)rand());
#else
  word_t num = (word_t)rand();
#endif

  if (rand() % 2) {
    c_buf_pos += sprintf(c_buf + c_buf_pos, "%" WORDdfmt suffix, num);
    expr_buf_pos += sprintf(expr_buf + expr_buf_pos, "%" WORDdfmt, num);
  } else {
    c_buf_pos += sprintf(c_buf + c_buf_pos, "0x%" WORDxfmt suffix, num);
    expr_buf_pos += sprintf(expr_buf + expr_buf_pos, "0x%" WORDxfmt, num);
  }

  token_num++;
  return;
}

static void gen(char s) {
  c_buf[c_buf_pos++] = s;
  expr_buf[expr_buf_pos++] = s;
  token_num++;
}

static void gen_rand_op(int level) {
  token_num++;
  while (1){
    int op = rand() % level;
    switch (op) {
      case 0: c_buf[c_buf_pos++] = '+'; expr_buf[expr_buf_pos++] = '+'; return;
      case 1: c_buf[c_buf_pos++] = '-'; expr_buf[expr_buf_pos++] = '-'; return;
      case 2: c_buf[c_buf_pos++] = '*'; expr_buf[expr_buf_pos++] = '*'; return;
      case 3: c_buf[c_buf_pos++] = '/'; expr_buf[expr_buf_pos++] = '/'; return;
      case 4: c_buf[c_buf_pos++] = '%'; expr_buf[expr_buf_pos++] = '%'; return;
      case 5: c_buf[c_buf_pos++] = '&'; expr_buf[expr_buf_pos++] = '&'; return;
      case 6: c_buf[c_buf_pos++] = '|'; expr_buf[expr_buf_pos++] = '|'; return;
      case 7: c_buf[c_buf_pos++] = '^'; expr_buf[expr_buf_pos++] = '^'; return;
      default:;
    }
  }
}

static void gen_rand_expr() {
  // int seed = time(0);
  // srand(seed);
  fuc_num++;
  if (fuc_num >= 8) {
    gen_num();
    fuc_num--;
    return;
  }
  switch (rand() % 8) {
    case 0:
      gen_num();
      break;
    case 1:
      gen(' ');
      gen_rand_expr();
      break;
    case 2:
      gen('(');
      gen('-');
      gen(' ');
      gen_rand_expr();
      gen(')');
      token_num += 3;
      break;
    case 3:
      gen('(');
      gen_rand_expr();
      gen(')');
      token_num += 2;
      break;
    default:
      gen_rand_expr();
      gen_rand_op(10);
      gen_rand_expr();
      break;
  }
  fuc_num--;
  return;
}

int main(int argc, char *argv[]) {
  srand(time(0));

  int loop = 1;
  FILE *res_file = NULL;
  if (argc <= 1) {
    printf("require the number of loops");
    assert(0);
  } else {
    sscanf(argv[1], "%d", &loop);
  }
  if (argv[2] != NULL) {
    res_file = fopen(argv[2], "w");
    assert(res_file != NULL);
  }

  int i;
  for (i = 0; i < loop; i++) {
    c_buf_pos = 0;
    token_num = 0;
    expr_buf_pos = 0;
    fuc_num = 0;

    gen_rand_expr();
    c_buf[c_buf_pos] = '\0';
    expr_buf[expr_buf_pos] = '\0';

    if (token_num >= 1024 || token_num <= 4)
      continue;

    sprintf(code_buf, code_format1, c_buf);
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    // fputs(code_format2, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr");
    if (ret != 0)
      continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    uint64_t result;
    ret = fscanf(fp, "%lx", &result);
    pclose(fp);

    if (res_file != NULL)
      fprintf(res_file, "%lx %s\n", result, expr_buf);
    else
      printf("%lx %s\n", result, expr_buf);
  }
  return 0;
}
