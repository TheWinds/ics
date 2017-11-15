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

static uint64_t str2uint64(char *str){
  uint64_t n=0;
  int lenStr=strlen(str);
  for(int i=0;i<lenStr;i++){
      n+=str[i]-48;
      if(i!=lenStr-1) n*=10;
  }
  return n;
}

static int cmd_si(char *args){
  // get the first argument 
  char *arg=strtok(NULL," ");
  uint64_t n=1;
  if (arg!=NULL){
      // parset string to unit64
      n = str2uint64(arg);
  }
  cpu_exec(n);
	return 0;
}

static void print_cmd_info_usage(){
    printf("info usage:\n");
    printf("  info r : print registers info \n");
    printf("  info w : print watchpointer info \n");
}

static void print_registers_info(){
    for(int i=R_EAX;i<=R_EDI;i++){
      printf("%s: 0x%08x\n",regsl[i],reg_l(i));  
    } 
}

static int cmd_info(char *args){
  char *arg=strtok(NULL," ");
  if(arg==NULL){
    print_cmd_info_usage();
    return 0;
  }

  if(strcmp(arg,"r")==0){
    // print registers info 
    print_registers_info();
  }
  else if(strcmp(arg,"w")==0){   
    
  }else{
    print_cmd_info_usage();
  }
  return 0;  
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si","Step execute N instruction,si [N] ",cmd_si},
  {"info","print registers or watchpointer information",cmd_info},

  /* TODO: Add more commands */

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
