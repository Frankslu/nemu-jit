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
#include <isa.h>
#include <memory/vaddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <setjmp.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NE,
  TK_NUM, TK_REG, TK_LAND, TK_LOR,
  TK_SL, TK_SRL, TK_SRA, TK_GEU,
  TK_LEU, TK_GS, TK_LS, TK_GES,
  TK_LES, TK_END
};

static struct rule {
  const char *regex;
  const int token_type;
  const char *token_name;
} rules[] = {
    // variable
    {"0x[0-9a-f]+", TK_NUM, "number"},
    {"0X[0-9A-F]+", TK_NUM, "number"},
    {"[0-9]+", TK_NUM, "number"},
    {"\\$\\w++", TK_REG, "reg"},
    // arithmetic
    {"\\+", '+', "+"},
    {"-", '-', "-"},
    {"\\*", '*', "*"},
    {"/", '/', "/"},
    {"%", '%', "%"},
    {"<<", TK_SL, "<<"},
    {">>", TK_SRL, ">>"},
    {"s>>", TK_SRA, "s>>"},
    {"<=", TK_GEU, "<="},
    {">=", TK_LEU, ">="},
    {"<", '<', "<"},
    {">", '>', ">"},
    {"s<=", TK_GES, "s<="},
    {"s>=", TK_LES, "s>="},
    {"s<", TK_GS, "s<"},
    {"s>", TK_LS, "s>"},
    // logical
    {"&&", TK_LAND, "&&"},
    {"\\|\\|", TK_LOR, "||"},
    {"==", TK_EQ, "=="},
    {"!=", TK_NE, "!="},
    {"!", '!', "!"},
    // bit
    {"&", '&', "&"},
    {"\\|", '|', "|"},
    {"\\^", '^', "^"},
    {"~", '~', "~"},
    // miscellaneous
    {"\\(", '(', "("},          // brackets
    {"\\)", ')', ")"},          // brackets
    {" +", TK_NOTYPE, "space"}, // spaces
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

static int get_token_num(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  int nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len,
        //     substr_start);

        position += substr_len;

        int type = rules[i].token_type;
        unsigned long long num;
        Token a;
        bool success;
        switch (type) {
          case TK_NOTYPE:
            break;

          case TK_REG:
            if (substr_len >= sizeof(a.name)) {
              printf("Token str too long for token.name to store\n");
              goto err;
            }
            strncpy(a.name, substr_start, substr_len);
            a.name[substr_len] = '\0';
            if (({
                  isa_reg_str2val(&a.name[1], &success);
                  success == false;
                })) {
              printf("reg name error:\n");
              position -= substr_len;
              goto err;
            }
            nr_token++;
            break;

          case TK_NUM:
            num = strtoull(substr_start, NULL, 0);
            if (num > WORD_MAX) {
              printf("Number is too large that word_t cannot receive");
              goto err;
            }
            nr_token++;
            break;

          default:
            if (substr_len >= sizeof(a.name)) {
              printf("Token str too long for token.name to store\n");
              goto err;
            }
            nr_token++;
            break;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
    err:
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return 0;
    }
  }

  return nr_token;
}

static void make_token(char *e, Token *tokens) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  int nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len,
        //     substr_start);

        position += substr_len;

        int type = rules[i].token_type;
        switch (type) {
        case TK_NOTYPE:
          break;

        case TK_REG:
          strncpy(tokens[nr_token].name, substr_start, substr_len);
          tokens[nr_token].name[substr_len] = '\0';
          tokens[nr_token++].type = type;
          break;

        case TK_NUM:
          strcpy(tokens[nr_token].name, "num");
          tokens[nr_token].type = type;
          tokens[nr_token++].val.num = strtoull(substr_start, NULL, 0);
          break;

        default:
          strncpy(tokens[nr_token].name, substr_start, substr_len);
          tokens[nr_token].name[substr_len] = '\0';
          tokens[nr_token++].type = type;
        }
        break;
      }
    }
  }
  tokens[nr_token] =
      (Token){.type = TK_END, .name = "end", .unary = false, .val.num = 0};
}

typedef struct ast_node {
  Token tk;
  struct ast_node *left, *right;
} Ast;

static jmp_buf expr_env;
static Token *token_ptr, *suffix_ptr;
static Ast *tree_ptr;

static Ast *E(Ast *node, int level);
static Ast *EP(Ast *node, int level);

static inline int binary_op_level(int type) {
  switch (type) {
    case '*':
    case '/':
    case '%': return 2;

    case '+':
    case '-': return 3;

    case TK_SL:
    case TK_SRA:
    case TK_SRL: return 4;

    case '<':
    case TK_GEU:
    case '>':
    case TK_LEU:
    case TK_GES:
    case TK_LES:
    case TK_GS:
    case TK_LS: return 5;

    case TK_EQ:
    case TK_NE: return 6;

    case '&': return 7;

    case '^': return 8;

    case '|': return 9;

    case TK_LAND: return 10;

    case TK_LOR: return 11;

    default:
      printf("Unknow binary op\n");
      longjmp(expr_env, 1);
  }
}

static inline bool is_unary_op(int type) {
  switch (type) {
    case '+':
    case '-':
    case '*':
    case '!':
    case '~': return true;
    default: return false;
  }
}

static inline Ast *alloc_ast(Token *token, bool unary) {
  tree_ptr->tk = *token;
  tree_ptr->tk.unary = unary;
  return tree_ptr++;
}

static const char *get_token_name(int type) {
  for (int i = 0; i < ARRLEN(rules); i++)
    if (type == rules[i].token_type)
      return rules[i].token_name;
  Assert(0, "Type doesn't exist\n");
}

static inline Token *match(int type) {
  Token *this = token_ptr++;
  if (this->type != type) {
    printf("Token type mismatch\n"
           "require: %s, actual: %s\n",
           get_token_name(type), this->name);
    longjmp(expr_env, 1);
  }
  return this;
}

static Ast *EP(Ast *node, int level) {
  Ast *father;
  int type = token_ptr->type;
  if (type == TK_END || type == ')' || binary_op_level(type) > level) {
    return node;
  } else if (binary_op_level(type) == level) {
    father = alloc_ast(match(type), false);
    father->left = node;
    father->right = E(node, level - 1);
    return EP(father, level);
  } else {
    printf("Grammar error at EP%d\n", level);
    longjmp(expr_env, 1);
  }
}

static Ast *E(Ast *node, int level) {
  int type = token_ptr->type;

  if (level != 1) {
    if (is_unary_op(type) || type == '(' || type == TK_NUM || type == TK_REG) {
      node = E(node, level - 1);
      return EP(node, level);
    } else {
      printf("Grammar error at E%d\n", level);
      longjmp(expr_env, 1);
    }
  } else {
    if (is_unary_op(type)) {
      node = alloc_ast(match(type), true);
      node->left = NULL;
      node->right = E(node, 1);
      return node;
    } else if (type == '(') {
      match('(');
      node = E(node, 11);
      match(')');
      return node;
    } else if (type == TK_NUM || type == TK_REG) {
      node = alloc_ast(match(type), false);
      node->left = NULL;
      node->right = NULL;
      return node;
    } else {
      printf("Grammar error at E%d\n", level);
      longjmp(expr_env, 1);
    }
  }
}

static Ast *make_ast(Ast *node) {
  node = E(node, 11);
  match(TK_END);
  return node;
}

static void ast_to_suffix(Ast *node) {
  int type = node->tk.type;
  bool unary = node->tk.unary;
  if (type == TK_NUM || type == TK_REG) {
    ;
  } else if (unary == true) {
    ast_to_suffix(node->right);
  } else {
    ast_to_suffix(node->left);
    ast_to_suffix(node->right);
  }
  *(suffix_ptr++) = node->tk;
}

Token *make_suffix(char *e) {
  int nr_token = get_token_num(e);
  if (nr_token == 0) {
    printf("token num is 0\n");
    return NULL;
  }

  suffix_ptr = (Token *)malloc((nr_token + 1) * sizeof(Token));
  token_ptr = (Token *)malloc((nr_token + 1) * sizeof(Token));
  tree_ptr = (Ast *)malloc(nr_token * sizeof(Ast));
  Token *tokens_head = token_ptr;
  Token *suffix_head = suffix_ptr;
  Ast *tree_head = tree_ptr;
  Ast *root = NULL;
  make_token(e, token_ptr);

  int err = setjmp(expr_env);
  if (err == 0) {
    root = make_ast(root);
    ast_to_suffix(root);
    *suffix_ptr =
        (Token){.type = TK_END, .name = "end", .unary = false, .val.num = 0};
    free(tokens_head);
    free(tree_head);
    return suffix_head;
  } else if (err == 1) {
    token_ptr--;
    int blank_len = 0, i = 0;
    for (i = 0; &tokens_head[i] != token_ptr; i++)
      if (tokens_head[i].type == TK_NUM)
        blank_len += printf(FMT_WORD_2 " ", tokens_head[i].val.num);
      else
        blank_len += printf("%s ", tokens_head[i].name);

    for (; i < nr_token; i++)
      if (tokens_head[i].type == TK_NUM)
        printf(FMT_WORD_2 " ", tokens_head[i].val.num);
      else
        printf("%s ", tokens_head[i].name);

    printf("\n%*.s^\n", blank_len, "");

    free(suffix_ptr);
    free(tokens_head);
    free(tree_head);

    return NULL;
  } else {
    Assert(0, "Unknown err\n");
  }
}

bool isVal(Token token) {
  return token.type == TK_NUM || token.type == TK_REG;
}

word_t getVal(Token token) {
  if (token.type == TK_NUM)
    return token.val.num;
  else if (token.type == TK_REG) {
    // we have checked reg at get_token_num, so we don't check it again
    return isa_reg_str2val(&token.name[1], &(bool){true});
  }
  else {
    printf("Should be number or reg\n");
    assert(0);
  }
}

word_t unary_val(word_t val) {
  switch ((suffix_ptr++)[0].type) {
    case '+': return val;
    case '-': return -val;
    case '*': {
      word_t res = vaddr_read(val, sizeof(word_t));
      if (is_oob()) {
        longjmp(expr_env, 1);
      }
      return res;
    }
    case '!': return !val;
    case '~': return ~val;
    default: Assert(0, "Should be binary op here\ntype: %s\n", suffix_ptr[-1].name);
  }
}

word_t binary_val(word_t current, word_t next) {
  switch ((suffix_ptr++)[0].type) {
    case '+': return current + next;
    case '-': return current - next;
    case '*': return current * next;
    case '/':
      if (next == 0) {
        Log("Divide by zero\n");
        longjmp(expr_env, 1);
      }
      return current / next;
    case '%':
      if (next == 0) {
        Log("Divide by zero\n");
        longjmp(expr_env, 1);
      }
      return current % next;
    case TK_EQ:   return current == next;
    case TK_NE:   return current != next;
    case TK_SL:   return current << next;
    case TK_SRL:  return current >> next;
    case TK_SRA:  return (sword_t)current >> next;
    case '<':     return current < next;
    case TK_GEU:  return current <= next;
    case '>':     return current > next;
    case TK_LEU:  return current >= next;
    case TK_GS:   return (sword_t)current < (sword_t)next;
    case TK_GES:  return (sword_t)current <= (sword_t)next;
    case TK_LS:   return (sword_t)current > (sword_t)next;
    case TK_LES:  return (sword_t)current >= (sword_t)next;
    case TK_LAND: return current && next;
    case TK_LOR:  return current || next;
    case '&':     return current & next;
    case '|':     return current | next;
    case '^':     return current ^ next;
    default:
      Assert(0, "Should be binary op here\ntype: %s\n", suffix_ptr[-1].name);
  }
}

static word_t _eval(word_t val1, word_t val2) {
  if (suffix_ptr[0].type == TK_END)
    return val2;
  else if (isVal(suffix_ptr[0]))
    val2 = _eval(val2, getVal((suffix_ptr++)[0]));
  else if (suffix_ptr[0].unary == true)
    val2 = unary_val(val2);
  else
    return binary_val(val1, val2);
 
  return _eval(val1, val2);
}

word_t eval(Token *suffix_expr, bool *success) {
  not_exit_on_oob();
  *success = true;
  suffix_ptr = suffix_expr;
  int err = setjmp(expr_env);
  if (err == 0) {
    word_t res = _eval(0, 0);
    return res;
  } else {
    *success = false;
    return 0;
  }
}

word_t expr(char *e, bool *success) {
  *success = true;
  suffix_ptr = make_suffix(e);
  if (suffix_ptr == NULL) {
    *success = false;
    return 0;
  }
  Token *suffix_head = suffix_ptr;
  word_t res = eval(suffix_ptr, success);
  free(suffix_head);
  return res;
}

// word_t eval(Ast *node) {
//   int type = node->tk.type;
//   if (node->left == NULL && node->right == NULL) {
//     switch (type) {
//     case TK_NUM:
//       return node->tk.val.num;
//     case TK_REG:
//       return *node->tk.val.reg;
//     default:
//       Assert(0, "Leaf node should be NUM or reg\ntype: %s\n", node->tk.name);
//     }
//   } else if (node->left == NULL && node->right != NULL) {
//     switch (type) {
//     case '+':
//       return eval(node->right);
//     case '-':
//       return -eval(node->right);
//     case '*':
//       return vaddr_read(eval(node->right), 4);
//     case '!':
//       return !eval(node->right);
//     case '~':
//       return ~eval(node->right);
//     default:
//       Assert(0, "left null node type err\ntype: %s\n", node->tk.name);
//     }
//   } else {
//     Ast *left = node->left;
//     Ast *right = node->right;
//     word_t divisor;
//     switch (type) {
//     case '+':
//       return eval(left) + eval(right);
//     case '-':
//       return eval(left) - eval(right);
//     case '*':
//       return eval(left) * eval(right);
//     case '/':
//       if ((divisor = eval(right)) == 0) {
//         printf("Divide by zero\n");
//         longjmp(expr_env, 1);
//       }
//       return eval(left) / eval(right);
//     case '%':
//       if ((divisor = eval(right)) == 0) {
//         printf("Divide by zero\n");
//         longjmp(expr_env, 1);
//       }
//       return eval(left) % eval(right);
//     case TK_EQ:
//       return eval(left) == eval(right);
//     case TK_NE:
//       return eval(left) != eval(right);
//     case TK_SL:
//       return eval(left) << eval(right);
//     case TK_SRL:
//       return eval(left) >> eval(right);
//     case TK_SRA:
//       return (sword_t)eval(left) >> eval(right);
//     case '<':
//       return eval(left) < eval(right);
//     case TK_GEU:
//       return eval(left) <= eval(right);
//     case '>':
//       return eval(left) > eval(right);
//     case TK_LEU:
//       return eval(left) >= eval(right);
//     case TK_GS:
//       return (sword_t)eval(left) < (sword_t)eval(right);
//     case TK_GES:
//       return (sword_t)eval(left) <= (sword_t)eval(right);
//     case TK_LS:
//       return (sword_t)eval(left) > (sword_t)eval(right);
//     case TK_LES:
//       return (sword_t)eval(left) >= (sword_t)eval(right);
//     case TK_LAND:
//       return eval(left) && eval(right);
//     case TK_LOR:
//       return eval(left) || eval(right);
//     case '&':
//       return eval(left) & eval(right);
//     case '|':
//       return eval(left) | eval(right);
//     case '^':
//       return eval(left) ^ eval(right);
//     default:
//       Assert(0, "middle node type err\ntype: %s\n", node->tk.name);
//     }
//   }
// }

// word_t expr(char *e, bool *success) {
//   *success = true;
//   int nr_token = get_token_num(e);
//   if (nr_token == 0) {
//     printf("token num is 0\n");
//     *success = false;
//     return 0;
//   }

//   tokens = (Token *)malloc((nr_token + 1) * sizeof(Token));
//   tree = (Ast *)malloc(nr_token * sizeof(Ast));
//   Ast *root = NULL;

//   make_token(e, tokens);

//   Token *tokens_head = tokens;
//   Ast *tree_head = tree;
//   int err = setjmp(expr_env);
//   int blank_len = 0, i = 0;
//   word_t res = 0;
//   if (err == 0) {
//     root = make_ast(root);
//     err = setjmp(expr_env);
//     if (err == 0) {
//       res = eval(root);
//       printf(FMT_WORD "\n", fmt_word(res));
//     } else if (err == 1) {
//       *success = false;
//     }
//   } else if (err == 1) {
//     tokens--;
//     for (i = 0; &tokens_head[i] != tokens; i++)
//       if (tokens_head[i].type == TK_NUM)
//         blank_len += printf(FMT_WORD_2 " ", tokens_head[i].val.num);
//       else
//         blank_len += printf("%s ", tokens_head[i].name);

//     for (; i < nr_token; i++)
//       if (tokens_head[i].type == TK_NUM)
//         printf(FMT_WORD_2 " ", tokens_head[i].val.num);
//       else
//         printf("%s ", tokens_head[i].name);

//     printf("\n%*.s^\n", blank_len, "");
//     *success = false;
//   } else {
//     Assert(0, "Unknown err\n");
//   }
//   free(tokens_head);
//   free(tree_head);

//   return res;
// }
