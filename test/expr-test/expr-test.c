#include <common.h>
#ifdef CONFIG_EXPR_TEST

extern void init_regex();
extern word_t expr(char *e, bool *success);

int main(int argc, char *argv[]) {
  init_regex();

  if (argc <= 1) {
    printf("need input.txt path\n");
    assert(0);
  }

  FILE *fp;
  fp = fopen(argv[1], "r");
  if (fp == NULL) {
    printf("%s not found\n", argv[1]);
    assert(0);
  }

  int total_run = 0, err = 0;
  word_t ref;
  char e[8192] = {};
  bool success = true;

  while (fscanf(fp, "%"MUXDEF(CONFIG_ISA64, PRIx64, PRIx32)" %8191[^\n]", &ref, e) == 2) {
    word_t dut = expr(e, &success);
    if ((word_t)ref != (word_t)dut && success == true) {
      printf("expr: %s\nref=" FMT_WORD "\ndut=" FMT_WORD "\n", e, fmt_word(ref), fmt_word(dut));
      printf("Total run finish:%d, err:%d\n", total_run, err);
      assert(0);
    }
    if (success == false || (word_t)ref != (word_t)dut) {
      printf("expr: %s\nref=" FMT_WORD "\ndut=" FMT_WORD "\n", e, fmt_word(ref), fmt_word(dut));
      err++;
    }
    printf("%-7d, dut: "FMT_WORD", ref: "FMT_WORD"\n", total_run, fmt_word(dut), fmt_word(ref));
    total_run++;
  }

  printf("Total run finish:%d, err:%d\n", total_run, err);
  return 0;
}
#endif