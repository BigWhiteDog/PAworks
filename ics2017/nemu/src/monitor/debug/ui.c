#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

#include<inttypes.h>
static int cmd_si(char *args) {
  
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    /* no argument given */
    cpu_exec(1);
  }
  else {
    uint64_t temp_n;
    int temp_flag;
    
    temp_flag=sscanf(arg,"%" PRIu64 ,&temp_n);
    
    if(temp_flag<=0)
      printf("Wrong input '%s'\n", arg);
    else
      cpu_exec(temp_n);
  }
  return 0;

}
static int cmd_info(char *args) {

  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    /* no argument given */
    printf("need w or r!\n");
  }
  else {
    if(arg[0]=='r'&&arg[1]=='\0')
    {
      printf("=============\n");
      printf("eip: %8x\n\n",cpu.eip);

      printf("eax: %8x\n",cpu.gpr[0]._32 );
      printf("edx: %8x\n",cpu.gpr[1]._32 );
      printf("ecx: %8x\n",cpu.gpr[2]._32 );
      printf("ebx: %8x\n",cpu.gpr[3]._32 );
      printf("ebp: %8x\n",cpu.gpr[4]._32 );
      printf("esi: %8x\n",cpu.gpr[5]._32 );
      printf("edi: %8x\n",cpu.gpr[6]._32 );
      printf("esp: %8x\n",cpu.gpr[7]._32 );

    }
    else if(arg[0]=='w'&&arg[1]=='\0')
    {

    }
    else
    {
      printf("wrong argument!\n");
    }
  }
  return 0;
}
static int cmd_p(char *args) {
  return 0;
}
static int cmd_x(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    /* no argument given */
    printf("need arguments!\n");
  }
  else{
    char *charp_n=arg;
    char *charp_expr=strtok(NULL, " ");
    uint temp_n;
    int scan_flag;
    scan_flag=sscanf(charp_n,"%u",&temp_n);
    if(scan_flag<=0)
    {
      printf("wrong N!\n");
      return 0;
    }
    paddr_t addr;

    uint i;
    for (i = 0; i < temp_n; ++i)
    {
      printf("%8x\n",paddr_read(addr,4));
      addr+=4;
    }
  }
  return 0;
}
static int cmd_w(char *args) {
  return 0;
}
static int cmd_d(char *args) {
  return 0;
}
static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si","step n instructions",cmd_si},
  {"info","print target reg or watchpoint",cmd_info},
  {"p","print the value of an EXPR",cmd_p},
  {"x","scan the content of target Mem Addr in N words",cmd_x},
  {"w","watch an EXPR, pause when it changes",cmd_w},
  {"d","delete watchpoint N",cmd_d}

  /* TODO: Add more commands */
  /*done*/
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
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

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
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

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
