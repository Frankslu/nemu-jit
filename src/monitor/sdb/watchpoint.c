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

#include "sdb.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char *expr_str;
  word_t res;
  word_t old_res;
  Token *suffix;
} WP;

#define NR_WP 32
#define NR_POOL NR_WP

#define T WP
#define pool_name wp_pool
#define pool_head head
#define pool_free free_

#define func_init_chain_name init_wp_pool

#define func_insert_chain_name insert_wp
#define func_insert_chain_static

#define func_find_prev_chain_name findprev_wp
#define func_find_prev_chain_static

#include <template/orderd-pool.h>

int new_wp(char *s) {
  bool success;
  Token *suffix = make_suffix(s);
  if (suffix == NULL) {
    printf("Make suffix fail\n");
    return -1;
  }

  word_t res = eval(suffix, &success);
  if (success == false) {
    printf("Eval suffix fail\n");
    return -1;
  }

  WP *_new_wp = free_->next;
  if (_new_wp == NULL) {
    printf("Watchpoint pool is full\n");
    free(suffix);
    return -1;
  }
  free_->next = _new_wp->next;

  insert_wp(_new_wp, head);

  _new_wp->expr_str = stralloc(s);
  _new_wp->res = res;
  _new_wp->old_res = res;
  _new_wp->suffix = suffix;
  printf("Watchpoint value: " FMT_WORD "\n", fmt_word(_new_wp->res));
  return _new_wp->NO;
}

bool free_wp(int NO) {
  if (NO == -1) {
    WP *p = head->next;
    if (p != NULL) {
      printf("watchpoint %d has beed deleted\n    expr: %s\n", p->NO,
             p->expr_str);
      head->next = p->next;
      free(p->expr_str);
      free(p->suffix);
      insert_wp(p, free_);
      return free_wp(-1);
    } else {
      return true;
    }
  } else {
    WP *prev = findprev_wp(NO, head);
    if (prev == NULL) {
      printf("Watchpoint %d not found\n", NO);
      return false;
    }
    WP *p = prev->next;
    printf("watchpoint %d has beed deleted\n    expr: %s\n", p->NO,
           p->expr_str);
    prev->next = p->next;
    free(p->expr_str);
    free(p->suffix);

    insert_wp(p, free_);
    return true;
  }
}

void scan_watchpoint(vaddr_t pc) {
  for (WP *p = head->next; p != NULL; p = p->next) {
    bool success;
    word_t new_res = eval(p->suffix, &success);
    if (success == false) {
      printf("watchpint eval fail\n");
      continue;
    }

    if (p->old_res != new_res) {
      printf("\nHint watchpoint %d at address " FMT_WORD ", expr = %s\n", p->NO, fmt_word(pc), p->expr_str);
      printf("old value = " FMT_WORD "\nnew value = " FMT_WORD "\n", fmt_word(p->old_res), fmt_word(new_res));
      p->old_res = new_res;
      if (nemu_state.state == NEMU_RUNNING)
        nemu_state.state = NEMU_STOP;
    }
  }
}

void display_watchpoint() {
  printf("\n");

  for (WP *p = head->next; p != NULL; p = p->next)
    printf("watchpoint %2d:\n    expr: %s\n    value: " FMT_WORD "\n", p->NO,
           p->expr_str, fmt_word(p->res));

  printf("\n");
}