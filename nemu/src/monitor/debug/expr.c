#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256,
  TK_PLUS,TK_SUB,TK_MUL_DREF,TK_DIV,
  TK_LEFT_PARENTHESES,
  TK_RIGHT_PARENTHESES,
  TK_EQ,TK_NOT_EQ,
  TK_AND,TK_OR,TK_NOT,
  TK_NUMBER,TK_HEX,
  TK_REG
  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
    {"\\$[a-zA-Z]+", TK_REG},      // register name
    {"0x[0-9]+", TK_NUMBER},      // hex
    {"[0-9]+", TK_NUMBER},        // number
    {" +", TK_NOTYPE},            // spaces
    {"\\+", TK_PLUS},              // plus
    {"-", TK_SUB},                // sub
    {"\\*", TK_MUL_DREF},          // mul or deference
    {"/", TK_DIV},                // div
    {"\\(", TK_LEFT_PARENTHESES},  // left parentheses
    {"\\)", TK_RIGHT_PARENTHESES}, // right parentheses
    {"==", TK_EQ},                // equal
    {"!=", TK_NOT_EQ},            // not equal
    {"&&", TK_AND},               // and
    {"||", TK_OR},                // or
    {"!", TK_NOT}                 // not
};
#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )
static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        Token new_token;
        switch (rules[i].token_type) {
          // ignore space
          case TK_NOTYPE:break;
          case TK_PLUS:
            new_token.type=TK_PLUS;
            break;
          default: TODO();
        }
        tokens[nr_token]=new_token;
        nr_token++;

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
