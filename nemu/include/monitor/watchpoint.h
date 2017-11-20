#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expression[32*32];
  uint32_t value;

} WP;
void show_watchpoints();
bool del_wp(int n);
bool add_wp(char* expression);
#endif
