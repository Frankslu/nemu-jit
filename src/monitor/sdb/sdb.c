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
#include <cpu/cpu.h>
#include <memory/vaddr.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline(ANSI_FMT("(nemu) ", ANSI_FG_GREEN));

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(UINT64_MAX);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
  if (args == NULL)
    cpu_exec(1);
  else
    cpu_exec(strtoll(args, NULL, 0));

  return 0;
}

static int cmd_p(char *args) {
  bool success = true;
  if (args != NULL) {
    word_t res = expr(args, &success);
    if (success)
      printf(FMT_WORD"\n", fmt_word(res));
  }
  return 0;
}

static int cmd_info(char *args) {
	if (NULL == args){
		printf(ANSI_FMT("command error\n", ANSI_FG_RED));
		return 0;
	}

  struct {
    char *name;
    void (*handler)();
  } info_table [] = {
    {"r", isa_reg_display},
    {"w", display_watchpoint}
  };

  for (int i = 0; i < ARRLEN(info_table); i++)
    if (streq(info_table[i].name, args)) {
      info_table[i].handler();
      return 0;
    }

  printf("Unknown arg\n");
  return 0;
}

#ifdef CONFIG_WATCH_POINT
static int cmd_w(char *args) {
	if (NULL == args){
		printf(ANSI_FMT("command error\n", ANSI_FG_RED));
		return 0;
	}

  int NO;
  if ((NO = new_wp(args)) == -1)
    printf("Set watchpoint fail\n");
  else
    printf("Set watchpoint %2d\n", NO);

  return 0;
}
#endif

static int cmd_x(char *args){
	if (NULL == args){
		printf(ANSI_FMT("command error: need size\n", ANSI_FG_RED));
		return 0;
	}

	args = strtok(args, " ");
  int size = atoi(args);
	args = args + strlen(args) + 1;
  if (args == NULL) {
		printf(ANSI_FMT("command error: need expr\n", ANSI_FG_RED));
		return 0;
  }

	bool success = true;
	word_t result = expr(args, &success);
	if (success == false)
		return 0;

  not_exit_on_oob();
  for (int i = 0; i < size;i++){
    word_t ret = vaddr_read(result + 4 * i, 4);
    if (is_oob())
      return 0;
    printf("0x%08lx:  %08lx\n", result + 4 * i, ret);
  }

	return 0;
}

static int cmd_d(char *args) {
  if (args == NULL)
    free_wp(-1);
  else
    free_wp(atoi(args));

  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si", "step", cmd_si },
  {"p", "print", cmd_p },
  {"info", "print information", cmd_info},
  {"x", "scan memory", cmd_x},
  {"d", "delete watchpoint", cmd_d},
  IFDEF(CONFIG_WATCH_POINT,{"w", "set watchpoint", cmd_w},)
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  } else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf(ANSI_FMT("Unknown command ", ANSI_FG_RED)"'%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
