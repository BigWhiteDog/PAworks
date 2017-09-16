#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,
  
  /* TODO: Add more token types */
  TK_NEQ,TK_AND,TK_OR,TK_NOT,
  TK_NEG,TK_PTR_BY,
//all token >= NUM is a value
  NUM,HEX_NUM,

  TK_EAX=300,
  TK_EDX,
  TK_ECX,
  TK_EBX,
  TK_EBP,
  TK_ESI,
  TK_EDI,
  TK_ESP,
  TK_AX=308,
  TK_DX,
  TK_CX,
  TK_BX,
  TK_BP,
  TK_SI,
  TK_DI,
  TK_SP,
  TK_AL=316,TK_AH,
  TK_DL    ,TK_DH,
  TK_CL    ,TK_CH,
  TK_BL    ,TK_BH,

  TK_EIP=324
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */


  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\*",'*'},
  {"-",'-'},
  {"/",'/'},
  {"\\(",'('},
  {"\\)",')'},
  {"0x([0-9]|[a-f])+",HEX_NUM},
  {"[1-9][0-9]*",NUM},


  {"\\$eax",TK_EAX},
  {"\\$ax",TK_AX},
  {"\\$ah",TK_AH},
  {"\\$al",TK_AL},

  {"\\$edx",TK_EDX},
  {"\\$dx",TK_DX},
  {"\\$dh",TK_DH},
  {"\\$dl",TK_DL},

  {"\\$ecx",TK_ECX},
  {"\\$cx",TK_CX},
  {"\\$ch",TK_CH},
  {"\\$cl",TK_CL},

  {"\\$ebx",TK_EBX},
  {"\\$bx",TK_BX},
  {"\\$bh",TK_BH},
  {"\\$bl",TK_BL},

  {"\\$ebp",TK_EBP},
  {"\\$bp",TK_BP},

  {"\\$esi",TK_ESI},
  {"\\$si",TK_SI},

  {"\\$edi",TK_EDI},
  {"\\$di",TK_DI},

  {"\\$esp",TK_ESP},
  {"\\$sp",TK_SP},


  {"\\$eip",TK_EIP},


  {"==", TK_EQ},         // equal
  {"!=", TK_NEQ},
  {"&&",TK_AND},
  {"\\|\\|",TK_OR},
  {"!", TK_NOT}

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

        switch (rules[i].token_type) {
          case TK_NOTYPE:{
            nr_token--;
            break;
          }
          case HEX_NUM:{
            if (substr_len>10)
            {
              printf("too long hex number!\n");
              assert(0);
            }
            tokens[nr_token].type=rules[i].token_type;
            memcpy(tokens[nr_token].str,substr_start+2,substr_len-2);//remove 0x
            tokens[nr_token].str[substr_len-2]='\0';
            break;
          }
          case NUM:{
            if (substr_len>9)
            {
              printf("too long dec number!\n");
              assert(0);
            }
            tokens[nr_token].type=rules[i].token_type;
            memcpy(tokens[nr_token].str,substr_start,substr_len);
            tokens[nr_token].str[substr_len]='\0';
            break;
          }
          case '*':{
            if(nr_token!=0&&( tokens[nr_token-1].type>=NUM || tokens[nr_token-1].type==')' ))
              tokens[nr_token].type='*';
            else
              tokens[nr_token].type=TK_PTR_BY;
            break;
          }

          default: tokens[nr_token].type=rules[i].token_type;
        }
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

uint32_t my_atou(char *p){
  uint32_t val=0;
  while(*p)
    val=val*10+(*p++)-'0';
  return val;
}
uint32_t my_htou(char *p){
  uint32_t val=0;
  while(*p){
    if(*p>='a')
      val=val<<4+(*p++)-'a'+10;
    else
      val=val<<4+(*p++)-'0';
  }
  return val;
}
bool check_parentheses(int p,int q,char* err_flag){
  int match=1;
  if(tokens[p].type!='(')
    return false;
  while(p<q){
    int tk_type=tokens[p].type
    if (tk_type==')'){
      if(--match == 0)
        return false;
    }
    else if(tk_type=='(')
      match++;
    p++;
  }
  if(tokens[p].type==')' && match==1)
    return true;

  return false;
}
int prior_map(int tk_type)
{
  switch (tk_type)
  {
    case TK_PTR_BY: return 2;
    case TK_NOT   : return 2;
    case '*'      : return 3;
    case '/'      : return 3;
    case '+'      : return 4;
    case '-'      : return 4;
    case TK_EQ    : return 7;
    case TK_NEQ   : return 7;
    case TK_AND   : return 11;
    case TK_OR    : return 12;

    default: return 0;
  }
}
uint32_t eval(int p,int q, char *err_flag) {
  if (p > q) {
    *err_flag=1;
    return 0;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    int tk_type=tokens[p].type;
    if(tk_type<NUM){
      *err_flag=1;
      return 0;
    }
    if(tk_type==NUM)
      return my_atou(tokens[p].str);
    else if(tk_type==HEX_NUM)
      return my_htou(tokens[p].str);
    
    else if(tk_type<308)
      return cpu.gpr[tk_type-300]._32;
    else if(tk_type<316)
      return (uint32_t)(cpu.gpr[tk_type-308]._16);
    else if(tk_type<324)
      return (uint32_t)(cpu.gpr[(tk_type-316)>>1]._8[1u & (tk_type-316)]);
    else if (tk_type==324)
      return (uint32_t)(cpu.eip);   
    else{
      *err_flag=1;
      return 0;
    }
  }
  else if (check_parentheses(p, q, err_flag) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    if(*err_flag)//really unmatch
      return 0;
    return eval(p + 1, q - 1,err_flag);
  }
  else {
    char err_flag_1=0;
    char err_flag_2=0;
    int i,i_type,op,op_type=0;
    int match=0
    for(i=p;i<q;i++)
    {
      i_type=tokens[i].type;
      
      
      if(match==0)//outside of bracket
        if(i_type<NUM)//i is non-val
          if(prior_map(i_type)>=op_type){
            op=i;
            op_type=prior_map(i_type);
          }
      if(i_type=='(')
        match++;
      else if(i_type==')')
        match--;
      
    }

    if(op_type==0)//can't find domain op
      assert(0);
    
    op_type=tokens[op].type;

    if(op_type==TK_PTR_BY)
    {
      vaddr_t addr=eval(op+1,q,err_flag);
      if(*err_flag)//handle for no core dump
        return 0;
      return vaddr_read(addr,4);
    }else if(op_type==TK_NOT)
      return !(eval(op+1,q,err_flag));
    

    val1 = eval(p, op - 1,&err_flag_1);
    val2 = eval(op + 1,q ,&err_flag_2);
    
    if (err_flag_1||err_flag_2)
    {
      *err_flag=1;
      return 0;
    }

    switch (op_type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR: return val1 || val2;

      default: assert(0);
    }
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  char err_flag=0;
  uint32_t res=eval(0,nr_token-1,&err_flag);
  
  if (err_flag)
  {
    *success = false;
    return 0;
  }

  *success=true;
  return res;
}
