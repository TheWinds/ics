#include "nemu.h"
#include "utils.h"
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum
{
  TK_NOTYPE = 256,
  TK_LEFT_PARENTHESES,
  TK_RIGHT_PARENTHESES,
  TK_DREF,
  TK_NEG,
  TK_NOT,
  TK_MUL,
  TK_DIV,
  TK_PLUS,
  TK_SUB,
  TK_EQ,
  TK_NOT_EQ,
  TK_AND,
  TK_OR,
  TK_NUMBER,
  TK_HEX,
  TK_REG
};

static struct rule
{
  char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
    {"\\$[a-zA-Z]+", TK_REG},      // register name
    {"0x[0-9A-Za-z]+", TK_HEX},    // hex
    {"[0-9]+", TK_NUMBER},         // number
    {" +", TK_NOTYPE},             // spaces
    {"\\+", TK_PLUS},              // plus
    {"-", TK_SUB},                 // sub
    {"\\*", TK_MUL},               // mul or deference
    {"/", TK_DIV},                 // div
    {"\\(", TK_LEFT_PARENTHESES},  // left parentheses
    {"\\)", TK_RIGHT_PARENTHESES}, // right parentheses
    {"==", TK_EQ},                 // equal
    {"!=", TK_NOT_EQ},             // not equal
    {"&&", TK_AND},                // and
    {"\\|\\|", TK_OR},             // or
    {"!", TK_NOT}                  // not
};
#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))
static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;
char token_strs[TK_OR - TK_NOTYPE + 1][10] = {
    " ", "(", ")", " [*] ", " [-] ", "!", "*", "/", "+",
    "-", "==", "!=", "&&", "||"};
static char *get_token_str(int token_type)
{
  int index = token_type - TK_NOTYPE;

  return token_strs[index];
}

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        Token new_token;
        memset(new_token.str, 0, sizeof(new_token.str));
        int new_token_type = rules[i].token_type;
        // save token type
        if (new_token_type >= TK_NOTYPE && new_token_type <= TK_REG)
        {
          new_token.type = new_token_type;
        }
        // save token value
        switch (new_token_type)
        {
        case TK_REG:
          strncpy(new_token.str, substr_start + 1, substr_len - 1);
          break;
        case TK_NUMBER:
          strncpy(new_token.str, substr_start, substr_len);
          break;
        case TK_HEX:
          strncpy(new_token.str, substr_start + 2, substr_len - 2);
          break;
        // fix token type
        case TK_SUB:
          if (nr_token == 0 || (tokens[nr_token - 1].type >= TK_NEG && tokens[nr_token - 1].type <= TK_OR))
          {
            new_token.type = TK_NEG;
          }
          break;
        case TK_MUL:
          if (nr_token == 0 || (tokens[nr_token - 1].type >= TK_DREF && tokens[nr_token - 1].type <= TK_OR))
          {
            new_token.type = TK_DREF;
          }
          break;
        default:
          break;
        }
        if (new_token.type == TK_NOTYPE)
          break;
        tokens[nr_token] = new_token;
        nr_token++;

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int check_parentheses(int p, int q)
{
  int stack = 0;
  bool has_parentheses = false;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == TK_LEFT_PARENTHESES)
    {
      has_parentheses = true;
      stack++;
    }
    if (tokens[i].type == TK_RIGHT_PARENTHESES)
    {
      has_parentheses = true;
      stack--;
    }
    // too many right parentheses
    if (stack < 0)
    {
      return false;
    }
  }
  if (has_parentheses)
  {
    // too many left parentheses
    if (stack != 0)
    {
      return false;
    }
    return tokens[p].type == TK_LEFT_PARENTHESES && tokens[q].type == TK_RIGHT_PARENTHESES && (p != 0 || q != nr_token - 1);
  }

  return -1;
}

static int op_priority[TK_OR - TK_LEFT_PARENTHESES + 1] = {
    0, 0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 6};

int get_op_priority(int token_type)
{
  // * (dereference) - (negative) !(not)
  // * /
  // + -
  // == !=
  // &&
  // ||
  return op_priority[token_type - TK_LEFT_PARENTHESES];
}

int find_dominant_op(int p, int q)
{
  int parentheses = 0;
  int dominant_op = -1;
  for (int i = q; i >= p; i--)
  {
    int token_type = tokens[i].type;
    if (token_type == TK_RIGHT_PARENTHESES)
    {
      parentheses++;
      continue;
    }
    if (token_type == TK_LEFT_PARENTHESES)
    {
      parentheses--;
      continue;
    }
    // in parenthese
    if (parentheses != 0)
      continue;
    if (!(token_type >= TK_LEFT_PARENTHESES && token_type <= TK_OR))
      continue;
    // set default
    if (dominant_op == -1)
      dominant_op = i;
    int priority_new = get_op_priority(token_type);
    int priority_old = get_op_priority(tokens[dominant_op].type);

    if (priority_new > priority_old)
    {
      dominant_op = i;
    }
    // support unary operator
    if ((token_type == tokens[dominant_op].type) && token_type >= TK_DREF && token_type <= TK_NOT)
    {
      dominant_op = i;
    }
  }
  return dominant_op;
}



uint32_t register_val(char *str, bool *find)
{
  if(strcmp(str,"eip")==0){
    *find=true;
    return cpu.eip;
  }
  int i;
  for (i = R_EAX; i <= R_EDI; i++)
  {
    // Log("%u",reg_l(i));
    if (strcmp(regsl[i], str) == 0)
    {
      *find = true;
      return reg_l(i);
    }
  }
  return 0;
}

void show_eval_expr(int p, int q)
{
  Log("eval: ");
  for (int i = p; i <= q; i++)
  {
    Log(" ");
    int token_type = tokens[i].type;
    switch (token_type)
    {
    case TK_NUMBER:
      Log("%s", tokens[i].str);
      break;
    case TK_HEX:
      Log("0x%s", tokens[i].str);
      break;
    case TK_REG:
      Log("$%s", tokens[i].str);
      break;
    default:
      Log("%s", get_token_str(tokens[i].type));
      break;
    }
  }
  Log("\n");
}
bool eval_ok = true;
uint32_t eval(int p, int q)
{
  if (!eval_ok)
    return 0;

  if (p > q)
  {
    // bad expression
    printf("eval failed bad expression\n");
    eval_ok = false;
    return 0;
  }
  else if (p == q)
  {
    bool register_exist;
    uint32_t rval = 0;
    switch (tokens[p].type)
    {
    case TK_NUMBER:
      return str2uint32(tokens[p].str);
    case TK_HEX:
      return str2uint32_hex(tokens[p].str);
    case TK_REG:
      register_exist = false;
      rval = register_val(tokens[p].str, &register_exist);
      if (!register_exist)
      {
        printf("eval failed register $%s is not exist\n", tokens[p].str);
        eval_ok = false;
        return 0;
      }
      return rval;
    default:
      assert(0);
      return 0;
    }
  }
  else if (check_parentheses(p, q) == true)
  {
    return eval(p + 1, q - 1);
  }
  else
  {
    int dominant_op = find_dominant_op(p, q);
    if (dominant_op == -1)
    {
      printf("eval failed bad expression\n");
      eval_ok = false;
      return 0;
    }
    // Log("dominant_op :%s\n",get_token_str(tokens[dominant_op].type));
    int op_type = tokens[dominant_op].type;
    uint32_t val1 = 0, val2 = 0;
    if (op_type >= TK_DREF && op_type <= TK_NOT)
    {
      val1 = eval(dominant_op + 1, q);
    }
    else
    {
      val1 = eval(p, dominant_op - 1);
      val2 = eval(dominant_op + 1, q);
    }

    switch (op_type)
    {
    case TK_AND:
      return val1 && val2;
    case TK_OR:
      return val1 || val2;
    case TK_EQ:
      return val1 == val2;
    case TK_NOT_EQ:
      return val1 != val2;
    case TK_PLUS:
      return val1 + val2;
    case TK_SUB:
      return val1 - val2;
    case TK_MUL:
      return val1 * val2;
    case TK_DIV:
      if (val2 == 0)
      {
        printf("eval failed : the denominator can't be zero\n");
        eval_ok = false;
        return 0;
      }
      return val1 / val2;
    case TK_NEG:
      return -val1;
    case TK_NOT:
      return !val1;
    case TK_DREF:
      return vaddr_read(val1, 4);
    default:
      assert(0);
    }
  }
  return 0;
}

uint32_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }
  eval_ok = true;
  uint32_t result = eval(0, nr_token - 1);
  *success = eval_ok;
  return result;

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  return 0;
}
