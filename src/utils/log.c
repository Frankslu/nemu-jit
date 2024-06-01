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

#include <common.h>

extern uint64_t g_nr_guest_inst;

#ifndef CONFIG_TARGET_AM
FILE *log_fp[TAB_LEN(log)] = {0};

#define logfile_name(x) [GETID(x, log)] = str(GETID(x, log)),
const char *const log_file[] = {MAP(log_type, logfile_name)};

void init_log(const char *log_dir) {
  for_idx_in_table(i, log) {
    char file[256];
    log_fp[i] = stdout;
    if (log_dir != NULL) {
      unsigned long len = strlen(log_file[i]) + strlen(log_dir);
      Assert(len < 256, "log file name %s is too long, length is %lu",
             log_file[i], len);

      strncpy(file, log_dir, 255);
      strncat(file, log_file[i], 255);
      strncat(file, ".txt", 255);
      
      FILE *fp = fopen(file, "w");
      Assert(fp, "Can not open '%s'", log_dir);
      log_fp[i] = fp;
    }
    Log("%s is written to %s", log_file[i], log_dir ? file : "stdout");
  }
}

bool log_enable() {
  return MUXDEF(CONFIG_TRACE, (g_nr_guest_inst >= CONFIG_TRACE_START) &&
         (g_nr_guest_inst <= CONFIG_TRACE_END), false);
}
#endif
